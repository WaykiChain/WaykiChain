// Copyright (c) 2017-2019 The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "httpserver.h"
#include "rpc/core/rpcprotocol.h" // For HTTP status codes
#include "commons/util.h"

#include "config/chainparams.h"
#include <compat/compat.h>
#include <commons/util.h>
#include <netbase.h>
#include <init.h>
#include <sync.h>

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <condition_variable>

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/util.h>

#include <support/events.h>

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#endif

/** Maximum size of http request (request line + headers) */
static const size_t MAX_HEADERS_SIZE = 8192;

/** Used by SanitizeString() */
static const std::string CHARS_ALPHA_NUM = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/** Used by SanitizeString() */
enum SafeCharsRule {
    SAFE_CHARS_DEFAULT,     //!< The full set of allowed chars
    SAFE_CHARS_UA_COMMENT,  //!< BIP-0014 subset
    SAFE_CHARS_FILENAME,    //!< Chars allowed in filenames
    SAFE_CHARS_URI,         //!< Chars allowed in URIs (RFC 3986)
};

/** Used by SanitizeString() */
static const std::string SAFE_CHARS[] = {
    CHARS_ALPHA_NUM + " .,;-_/:?@()",             // SAFE_CHARS_DEFAULT
    CHARS_ALPHA_NUM + " .,;-_?@",                 // SAFE_CHARS_UA_COMMENT
    CHARS_ALPHA_NUM + ".-_",                      // SAFE_CHARS_FILENAME
    CHARS_ALPHA_NUM + "!*'();:@&=+$,/?#[]-_.~%",  // SAFE_CHARS_URI
};

std::string SanitizeString(const std::string& str, SafeCharsRule rule) {
    std::string strResult;
    for (std::string::size_type i = 0; i < str.size(); i++) {
        if (SAFE_CHARS[rule].find(str[i]) != std::string::npos) strResult.push_back(str[i]);
    }
    return strResult;
}

/** HTTP request work item */
class HTTPWorkItem final : public HTTPClosure {
public:
    HTTPWorkItem(std::unique_ptr<HTTPRequest> _req, const std::string& _path,
                 const HTTPRequestHandler& _func)
        : req(std::move(_req)), path(_path), func(_func) {}
    void operator()() override { func(req.get(), path); }

    std::unique_ptr<HTTPRequest> req;

private:
    std::string path;
    HTTPRequestHandler func;
};

/** Simple work queue for distributing work over multiple threads.
 * Work items are simply callable objects.
 */
template <typename WorkItem>
class WorkQueue {
private:
    /** Mutex protects entire object */
    StdMutex cs;
    std::condition_variable cond;
    std::deque<std::unique_ptr<WorkItem>> queue;
    bool running;
    size_t maxDepth;

public:
    explicit WorkQueue(size_t _maxDepth) : running(true), maxDepth(_maxDepth) {}
    /** Precondition: worker threads have all stopped (they have been joined).
     */
    ~WorkQueue() {}
    /** Enqueue a work item */
    bool Enqueue(WorkItem* item) {
        STD_LOCK(cs);
        if (queue.size() >= maxDepth) {
            return false;
        }
        queue.emplace_back(std::unique_ptr<WorkItem>(item));
        cond.notify_one();
        return true;
    }
    /** Thread function */
    void Run() {
        while (true) {
            std::unique_ptr<WorkItem> i;
            {
                STD_WAIT_LOCK(cs, lock);
                while (running && queue.empty()) cond.wait(lock);
                if (!running) break;
                i = std::move(queue.front());
                queue.pop_front();
            }
            (*i)();
        }
    }
    /** Interrupt and exit loops */
    void Interrupt() {
        STD_LOCK(cs);
        running = false;
        cond.notify_all();
    }
};

struct HTTPPathHandler {
    HTTPPathHandler(std::string _prefix, bool _exactMatch, HTTPRequestHandler _handler)
        : prefix(_prefix), exactMatch(_exactMatch), handler(_handler) {}
    std::string prefix;
    bool exactMatch;
    HTTPRequestHandler handler;
};

/** HTTP module state */

//! libevent event loop
static struct event_base* eventBase = nullptr;
//! HTTP server
struct evhttp* eventHTTP = nullptr;
//! List of subnets to allow RPC connections from
static std::vector<CSubNet> rpc_allow_subnets;
//! Work queue for handling longer requests off the event loop thread
static WorkQueue<HTTPClosure>* workQueue = nullptr;
//! Handlers for (sub)paths
std::vector<HTTPPathHandler> pathHandlers;
//! Bound listening sockets
std::vector<evhttp_bound_socket*> boundSockets;

//! main thread for http
std::thread threadHTTP;

//! thead workers
static std::vector<std::thread> g_thread_http_workers;

/** Check if a network address is allowed to access the HTTP server */
static bool ClientAllowed(const CNetAddr& netaddr) {
    if (!netaddr.IsValid()) return false;
    for (const CSubNet& subnet : rpc_allow_subnets)
        if (subnet.Match(netaddr)) return true;
    return false;
}

/** Initialize ACL list for HTTP server */
static bool InitHTTPAllowList() {
    rpc_allow_subnets.clear();
    CNetAddr localv4;
    CNetAddr localv6;
    LookupHost("127.0.0.1", localv4, false);
    LookupHost("::1", localv6, false);
    rpc_allow_subnets.push_back(CSubNet(localv4, 8));  // always allow IPv4 local subnet
    rpc_allow_subnets.push_back(CSubNet(localv6));     // always allow IPv6 localhost
    for (const std::string& strAllow : SysCfg().GetMultiArgs("-rpcallowip")) {
        CSubNet subnet;
        LookupSubNet(strAllow.c_str(), subnet);
        if (!subnet.IsValid()) {
            LogPrint("ERROR",
                     "Invalid -rpcallowip subnet specification: %s. Valid are a single IP "
                     "(e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a "
                     "network/CIDR (e.g. 1.2.3.4/24).",
                     strAllow);
            return false;
        }
        rpc_allow_subnets.push_back(subnet);
    }
    std::string strAllowed;
    for (const CSubNet& subnet : rpc_allow_subnets) {
        strAllowed += subnet.ToString() + " ";
    }
    LogPrint("RPC", "Allowing HTTP connections from: %s\n", strAllowed);
    return true;
}

/** HTTP request method as string - use for logging only */
static std::string RequestMethodString(HTTPRequest::RequestMethod m) {
    switch (m) {
        case HTTPRequest::GET:
            return "GET";
            break;
        case HTTPRequest::POST:
            return "POST";
            break;
        case HTTPRequest::HEAD:
            return "HEAD";
            break;
        case HTTPRequest::PUT:
            return "PUT";
            break;
        default:
            return "unknown";
    }
}

/** HTTP request callback */
static void http_request_cb(struct evhttp_request* req, void* arg) {
    // Disable reading to work around a libevent bug, fixed in 2.2.0.
    if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02020001) {
        evhttp_connection* conn = evhttp_request_get_connection(req);
        if (conn) {
            bufferevent* bev = evhttp_connection_get_bufferevent(conn);
            if (bev) {
                bufferevent_disable(bev, EV_READ);
            }
        }
    }
    std::unique_ptr<HTTPRequest> hreq(new HTTPRequest(req));

    // Early address-based allow check
    if (!ClientAllowed(hreq->GetPeer())) {
        LogPrint("RPC", "HTTP request from %s rejected: Client network is not allowed RPC access\n",
                 hreq->GetPeer().ToString());
        hreq->WriteReply(HTTP_FORBIDDEN);
        return;
    }

    // Early reject unknown HTTP methods
    if (hreq->GetRequestMethod() == HTTPRequest::UNKNOWN) {
        LogPrint("RPC", "HTTP request from %s rejected: Unknown HTTP request method\n",
                 hreq->GetPeer().ToString());
        hreq->WriteReply(HTTP_BADMETHOD);
        return;
    }

    LogPrint("RPC", "Received a %s request for %s from %s\n",
             RequestMethodString(hreq->GetRequestMethod()),
             SanitizeString(hreq->GetURI(), SAFE_CHARS_URI).substr(0, 100),
             hreq->GetPeer().ToString());

    // Find registered handler for prefix
    std::string strURI = hreq->GetURI();
    std::string path;
    std::vector<HTTPPathHandler>::const_iterator i    = pathHandlers.begin();
    std::vector<HTTPPathHandler>::const_iterator iend = pathHandlers.end();
    for (; i != iend; ++i) {
        bool match = false;
        if (i->exactMatch)
            match = (strURI == i->prefix);
        else
            match = (strURI.substr(0, i->prefix.size()) == i->prefix);
        if (match) {
            path = strURI.substr(i->prefix.size());
            break;
        }
    }

    // Dispatch to worker thread
    if (i != iend) {
        std::unique_ptr<HTTPWorkItem> item(new HTTPWorkItem(std::move(hreq), path, i->handler));
        assert(workQueue);
        if (workQueue->Enqueue(item.get()))
            item.release(); /* if true, queue took ownership */
        else {
            LogPrint("ERROR",
                     "WARNING: request rejected because http work queue depth exceeded, it can be "
                     "increased with the -rpcworkqueue= setting\n");
            item->req->WriteReply(HTTP_INTERNAL, "Work queue depth exceeded");
        }
    } else {
        hreq->WriteReply(HTTP_NOTFOUND);
    }
}

/** Callback to reject HTTP requests after shutdown. */
static void http_reject_request_cb(struct evhttp_request* req, void*) {
    LogPrint("RPC", "Rejecting request while shutting down\n");
    evhttp_send_error(req, HTTP_SERVUNAVAIL, nullptr);
}

/** Event dispatcher thread */
static bool ThreadHTTP(struct event_base* base) {
    RenameThread("bitcoin-http");
    LogPrint("RPC", "Entering http event loop\n");
    event_base_dispatch(base);
    // Event loop will be interrupted by InterruptHTTPServer()
    LogPrint("RPC", "Exited http event loop\n");
    return event_base_got_break(base) == 0;
}

/** Bind HTTP server to specified addresses */
static bool HTTPBindAddresses(struct evhttp* http) {
    int http_port = SysCfg().GetArg("-rpcport", SysCfg().RPCPort());
    std::vector<std::pair<std::string, uint16_t>> endpoints;

    // Determine what addresses to bind to
    if (SysCfg().IsArgCount("-rpcbind")) {
        // bind to specific address
        for (const std::string& strRPCBind : SysCfg().GetMultiArgs("-rpcbind")) {
            int port = http_port;
            std::string host;
            SplitHostPort(strRPCBind, port, host);
            endpoints.push_back(std::make_pair(host, port));
        }
    } else {  // no set -rpcbind
        if (SysCfg().IsArgCount("-rpcallowip")) {
            // bind to all ip
            endpoints.push_back(std::make_pair("::", http_port));
            endpoints.push_back(std::make_pair("0.0.0.0", http_port));
        } else {
            // bind to loopback ip only
            endpoints.push_back(std::make_pair("::1", http_port));
            endpoints.push_back(std::make_pair("127.0.0.1", http_port));
        }
    }

    // Bind addresses
    for (std::vector<std::pair<std::string, uint16_t>>::iterator i = endpoints.begin();
         i != endpoints.end(); ++i) {
        LogPrint("RPC", "Binding RPC on address %s port %i\n", i->first, i->second);
        evhttp_bound_socket* bind_handle = evhttp_bind_accept_socket(
            http, i->first.empty() ? nullptr : i->first.c_str(), i->second);
        if (bind_handle) {
            CNetAddr addr;
            boundSockets.push_back(bind_handle);
        } else {
            LogPrint("ERROR", "Binding RPC on address %s port %i failed.\n", i->first, i->second);
        }
    }
    return !boundSockets.empty();
}

/** Simple wrapper to set thread name and run work queue */
static void HTTPWorkQueueRun(WorkQueue<HTTPClosure>* queue) {
    RenameThread("bitcoin-httpworker");
    queue->Run();
}

/** libevent event log callback */
static void libevent_log_cb(int severity, const char* msg) {
#ifndef EVENT_LOG_WARN
// EVENT_LOG_WARN was added in 2.0.19; but before then _EVENT_LOG_WARN existed.
#define EVENT_LOG_WARN _EVENT_LOG_WARN
#endif
    if (severity >= EVENT_LOG_WARN) { // Log warn messages and higher without debug category
        LogPrint("LIBEVENT", "WARNING: libevent: %s\n", msg);
    } else {
        LogPrint("LIBEVENT", "libevent: %s\n", msg);
    }
}

bool InitHTTPServer() {
    if (!InitHTTPAllowList()) {
        return false;
    }

    // Redirect libevent's logging to our own log
    event_set_log_callback(&libevent_log_cb);
    /*
        // Update libevent's log handling. Returns false if our version of
        // libevent doesn't support debug logging, in which case we should
        // clear the BCLog::LIBEVENT flag.
        if (!UpdateHTTPServerLogging(LogInstance().WillLogCategory(BCLog::LIBEVENT))) {
            LogInstance().DisableCategory(BCLog::LIBEVENT);
        }
    */

#ifdef WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif

    raii_event_base base_ctr = obtain_event_base();

    /* Create a new evhttp object to handle requests. */
    raii_evhttp http_ctr = obtain_evhttp(base_ctr.get());
    struct evhttp* http  = http_ctr.get();
    if (!http) {
        LogPrint("ERROR", "couldn't create evhttp. Exiting.\n");
        return false;
    }

    evhttp_set_timeout(http, SysCfg().GetArg("-rpcservertimeout", DEFAULT_HTTP_SERVER_TIMEOUT));
    evhttp_set_max_headers_size(http, MAX_HEADERS_SIZE);
    evhttp_set_max_body_size(http, MAX_SIZE);
    evhttp_set_gencb(http, http_request_cb, nullptr);

    if (!HTTPBindAddresses(http)) {
        LogPrint("ERROR", "Unable to bind any endpoint for RPC server\n");
        return false;
    }

    LogPrint("RPC", "Initialized HTTP server\n");
    int workQueueDepth =
        std::max((long)SysCfg().GetArg("-rpcworkqueue", DEFAULT_HTTP_WORKQUEUE), 1L);
    LogPrint("RPC", "HTTP: creating work queue of depth %d\n", workQueueDepth);

    workQueue = new WorkQueue<HTTPClosure>(workQueueDepth);
    // transfer ownership to eventBase/HTTP via .release()
    eventBase = base_ctr.release();
    eventHTTP = http_ctr.release();
    return true;
}

bool UpdateHTTPServerLogging(bool enable) {
#if LIBEVENT_VERSION_NUMBER >= 0x02010100
    if (enable) {
        event_enable_debug_logging(EVENT_DBG_ALL);
    } else {
        event_enable_debug_logging(EVENT_DBG_NONE);
    }
    return true;
#else
    // Can't update libevent logging if version < 02010100
    return false;
#endif
}

void StartHTTPServer() {
    LogPrint("RPC", "Starting HTTP server\n");
    int rpcThreads = std::max((long)SysCfg().GetArg("-rpcthreads", DEFAULT_HTTP_THREADS), 1L);
    LogPrint("RPC", "HTTP: starting %d worker threads\n", rpcThreads);
    threadHTTP = std::thread(ThreadHTTP, eventBase);

    for (int i = 0; i < rpcThreads; i++) {
        g_thread_http_workers.emplace_back(HTTPWorkQueueRun, workQueue);
    }
}

void InterruptHTTPServer() {
    LogPrint("RPC", "Interrupting HTTP server\n");
    if (eventHTTP) {
        // Reject requests on current connections
        evhttp_set_gencb(eventHTTP, http_reject_request_cb, nullptr);
    }
    if (workQueue) workQueue->Interrupt();
}

void StopHTTPServer() {
    LogPrint("RPC", "Stopping HTTP server\n");
    if (workQueue) {
        LogPrint("RPC", "Waiting for HTTP worker threads to exit\n");
        for (auto& thread : g_thread_http_workers) {
            thread.join();
        }
        g_thread_http_workers.clear();
        delete workQueue;
        workQueue = nullptr;
    }
    // Unlisten sockets, these are what make the event loop running, which means
    // that after this and all connections are closed the event loop will quit.
    for (evhttp_bound_socket* socket : boundSockets) {
        evhttp_del_accept_socket(eventHTTP, socket);
    }
    boundSockets.clear();
    if (eventBase) {
        LogPrint("RPC", "Waiting for HTTP event thread to exit\n");
        threadHTTP.join();
    }
    if (eventHTTP) {
        evhttp_free(eventHTTP);
        eventHTTP = nullptr;
    }
    if (eventBase) {
        event_base_free(eventBase);
        eventBase = nullptr;
    }
    LogPrint("RPC", "Stopped HTTP server\n");
}

struct event_base* EventBase() {
    return eventBase;
}

static void httpevent_callback_fn(evutil_socket_t, short, void* data) {
    // Static handler: simply call inner handler
    HTTPEvent* self = static_cast<HTTPEvent*>(data);
    self->handler();
    if (self->deleteWhenTriggered) delete self;
}

HTTPEvent::HTTPEvent(struct event_base* base, bool _deleteWhenTriggered,
                     const std::function<void()>& _handler)
    : deleteWhenTriggered(_deleteWhenTriggered), handler(_handler) {
    ev = event_new(base, -1, 0, httpevent_callback_fn, this);
    assert(ev);
}

HTTPEvent::~HTTPEvent() {
     event_free(ev);
}

void HTTPEvent::trigger(struct timeval* tv) {
    if (tv == nullptr)
        event_active(ev, 0, 0);  // immediately trigger event in main thread
    else
        evtimer_add(ev, tv);  // trigger after timeval passed
}

HTTPRequest::HTTPRequest(struct evhttp_request* _req) : req(_req), replySent(false) {

}

HTTPRequest::~HTTPRequest() {
    if (!replySent) {
        // Keep track of whether reply was sent to avoid request leaks
        LogPrint("ERROR", "%s: Unhandled request\n", __func__);
        WriteReply(HTTP_INTERNAL, "Unhandled request");
    }
    // evhttpd cleans up the request, as long as a reply was sent.
}

std::pair<bool, std::string> HTTPRequest::GetHeader(const std::string& hdr) const {
    const struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
    assert(headers);
    const char* val = evhttp_find_header(headers, hdr.c_str());
    if (val)
        return std::make_pair(true, val);
    else
        return std::make_pair(false, "");
}

std::string HTTPRequest::ReadBody() {
    struct evbuffer* buf = evhttp_request_get_input_buffer(req);
    if (!buf) return "";
    size_t size = evbuffer_get_length(buf);
    /** Trivial implementation: if this is ever a performance bottleneck,
     * internal copying can be avoided in multi-segment buffers by using
     * evbuffer_peek and an awkward loop. Though in that case, it'd be even
     * better to not copy into an intermediate string but use a stream
     * abstraction to consume the evbuffer on the fly in the parsing algorithm.
     */
    const char* data = (const char*)evbuffer_pullup(buf, size);
    if (!data)  // returns nullptr in case of empty buffer
        return "";
    std::string rv(data, size);
    evbuffer_drain(buf, size);
    return rv;
}

void HTTPRequest::WriteHeader(const std::string& hdr, const std::string& value) {
    struct evkeyvalq* headers = evhttp_request_get_output_headers(req);
    assert(headers);
    evhttp_add_header(headers, hdr.c_str(), value.c_str());
}

/** Closure sent to main thread to request a reply to be sent to
 * a HTTP request.
 * Replies must be sent in the main loop in the main http thread,
 * this cannot be done from worker threads.
 */
void HTTPRequest::WriteReply(int nStatus, const std::string& strReply) {
    assert(!replySent && req);
    if (ShutdownRequested()) {
        WriteHeader("Connection", "close");
    }
    // Send event to main http thread to send reply message
    struct evbuffer* evb = evhttp_request_get_output_buffer(req);
    assert(evb);
    evbuffer_add(evb, strReply.data(), strReply.size());
    auto req_copy = req;
    HTTPEvent* ev = new HTTPEvent(eventBase, true, [req_copy, nStatus] {
        evhttp_send_reply(req_copy, nStatus, nullptr, nullptr);
        // Re-enable reading from the socket. This is the second part of the libevent
        // workaround above.
        if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02020001) {
            evhttp_connection* conn = evhttp_request_get_connection(req_copy);
            if (conn) {
                bufferevent* bev = evhttp_connection_get_bufferevent(conn);
                if (bev) {
                    bufferevent_enable(bev, EV_READ | EV_WRITE);
                }
            }
        }
    });
    ev->trigger(nullptr);
    replySent = true;
    req       = nullptr;  // transferred back to main thread
}

CService HTTPRequest::GetPeer() const {
    evhttp_connection* con = evhttp_request_get_connection(req);
    CService peer;
    if (con) {
        // evhttp retains ownership over returned address string
        const char* address = "";
        uint16_t port       = 0;
        evhttp_connection_get_peer(con, (char**)&address, &port);
        LookupNumeric(address, peer, port);
    }
    return peer;
}

std::string HTTPRequest::GetURI() const { return evhttp_request_get_uri(req); }

HTTPRequest::RequestMethod HTTPRequest::GetRequestMethod() const {
    switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET:
            return GET;
            break;
        case EVHTTP_REQ_POST:
            return POST;
            break;
        case EVHTTP_REQ_HEAD:
            return HEAD;
            break;
        case EVHTTP_REQ_PUT:
            return PUT;
            break;
        default:
            return UNKNOWN;
            break;
    }
}

void RegisterHTTPHandler(const std::string& prefix, bool exactMatch,
                         const HTTPRequestHandler& handler) {
    LogPrint("RPC", "Registering HTTP handler for %s (exactmatch %d)\n", prefix, exactMatch);
    pathHandlers.push_back(HTTPPathHandler(prefix, exactMatch, handler));
}

void UnregisterHTTPHandler(const std::string& prefix, bool exactMatch) {
    std::vector<HTTPPathHandler>::iterator i    = pathHandlers.begin();
    std::vector<HTTPPathHandler>::iterator iend = pathHandlers.end();
    for (; i != iend; ++i)
        if (i->prefix == prefix && i->exactMatch == exactMatch) break;
    if (i != iend) {
        LogPrint("RPC", "Unregistering HTTP handler for %s (exactmatch %d)\n", prefix, exactMatch);
        pathHandlers.erase(i);
    }
}

std::string urlDecode(const std::string& urlEncoded) {
    std::string res;
    if (!urlEncoded.empty()) {
        char* decoded = evhttp_uridecode(urlEncoded.c_str(), false, nullptr);
        if (decoded) {
            res = std::string(decoded);
            free(decoded);
        }
    }
    return res;
}
