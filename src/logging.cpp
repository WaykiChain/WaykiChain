// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logging.h"

#include "commons/util/threadnames.h"
#include "commons/util/time.h"
#include "commons/util/util.h"
#include "commons/types.h"

#include <mutex>

const char * const DEFAULT_DEBUGLOGFILE = "debug.log";

BCLog::Logger& LogInstance()
{
/**
 * NOTE: the logger instances is leaked on exit. This is ugly, but will be
 * cleaned up by the OS/libc. Defining a logger as a global object doesn't work
 * since the order of destruction of static/global objects is undefined.
 * Consider if the logger gets destroyed, and then some later destructor calls
 * LogPrintf, maybe indirectly, and you get a core dump at shutdown trying to
 * access the logger. When the shutdown sequence is fully audited and tested,
 * explicit destruction of these objects can be implemented by changing this
 * from a raw pointer to a std::unique_ptr.
 * Since the destructor is never called, the logger and all its members must
 * have a trivial destructor.
 *
 * This method of initialization was originally introduced in
 * ee3374234c60aba2cc4c5cd5cac1c0aefc2d817c.
 */
    static std::unique_ptr<BCLog::Logger> g_logger = std::make_unique<BCLog::Logger>();
    return *g_logger;
}

bool fLogIPs = DEFAULT_LOGIPS;

static int FileWriteStr(const std::string &str, FILE *fp)
{
    return fwrite(str.data(), 1, str.size(), fp);
}

bool BCLog::Logger::StartLogging()
{
    std::lock_guard<std::mutex> scoped_lock(m_cs);

    assert(m_buffering);
    assert(m_fileout == nullptr);

    if (m_print_to_file) {
        assert(!m_file_path.empty());
        m_fileout = fsbridge::fopen(m_file_path, "a");
        if (!m_fileout) {
            return false;
        }

        setbuf(m_fileout, nullptr); // unbuffered

        // Add newlines to the logfile to distinguish this execution from the
        // last one.
        FileWriteStr("\n\n\n\n\n", m_fileout);
    }

    // dump buffered messages from before we opened the log
    m_buffering = false;
    while (!m_msgs_before_open.empty()) {
        const std::string& s = m_msgs_before_open.front();

        if (m_print_to_file) FileWriteStr(s, m_fileout);
        if (m_print_to_console) fwrite(s.data(), 1, s.size(), stdout);
        for (const auto& cb : m_print_callbacks) {
            cb(s);
        }

        m_msgs_before_open.pop_front();
    }
    if (m_print_to_console) fflush(stdout);

    return true;
}

void BCLog::Logger::Flush() {
    if (Enabled()) {
        std::lock_guard<std::mutex> scoped_lock(m_cs);
        if (m_print_to_file && m_fileout != nullptr) {
            fflush(m_fileout);
        }
    }
}

void BCLog::Logger::DisconnectTestLogger()
{
    std::lock_guard<std::mutex> scoped_lock(m_cs);
    m_buffering = true;
    if (m_fileout != nullptr) fclose(m_fileout);
    m_fileout = nullptr;
    m_print_callbacks.clear();
}

void BCLog::Logger::EnableCategory(BCLog::LogFlags flag)
{
    m_categories |= flag;
}

bool BCLog::Logger::EnableCategory(const std::string& str)
{
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, str)) return false;
    EnableCategory(flag);
    return true;
}

void BCLog::Logger::DisableCategory(BCLog::LogFlags flag)
{
    m_categories &= ~flag;
}

bool BCLog::Logger::DisableCategory(const std::string& str)
{
    BCLog::LogFlags flag;
    if (!GetLogCategory(flag, str)) return false;
    DisableCategory(flag);
    return true;
}

uint64_t BCLog::Logger::GetCurrentLogSize() {

    // Scroll debug.log if it's getting too big
    FILE* file = fsbridge::fopen(m_file_path, "r");
    // Special files (e.g. device nodes) may not have a size.
    size_t log_size = 0;
    try {
        log_size = fs::file_size(m_file_path);
    } catch (const fs::filesystem_error&) {}

    if(file != nullptr)
        fclose(file);
    return log_size;

}

bool BCLog::Logger::WillLogCategory(BCLog::LogFlags category) const
{
    return (m_categories.load(std::memory_order_relaxed) & category) != 0;
}

bool BCLog::Logger::DefaultShrinkDebugFile() const
{
    return m_categories == BCLog::NONE;
}

struct CLogCategoryDesc
{
    BCLog::LogFlags flag;
    std::string category;
};


static const EnumTypeMap<BCLog::LogFlags, std::string, uint32_t> LOG_CATEGORY_MAP = {
    {BCLog::NONE,       "0"         },
    {BCLog::INFO,       "INFO"      },
    {BCLog::ERROR,      "ERROR"     },
    {BCLog::DEBUG,      "DEBUG"     },
    {BCLog::NET,        "NET"       },
    {BCLog::MINER,      "MINER"     },
    {BCLog::ALERT,      "ALERT"     },
    {BCLog::CDB,        "CDB"       },
    {BCLog::BDB,        "BDB"       },
    {BCLog::LDB,        "LDB"       },
    {BCLog::LUAVM,      "LUAVM"     },
    {BCLog::WASM,       "WASM"      },
    {BCLog::LOCK,       "LOCK"      },
    {BCLog::HTTP,       "HTTP"      },
    {BCLog::RPC,        "RPC"       },
    {BCLog::REINDEX,    "REINDEX"   },
    {BCLog::PROFIT,     "PROFIT"    },
    {BCLog::ADDRMAN,    "ADDRMAN"   },
    {BCLog::PRICEFEED,  "PRICEFEED" },
    {BCLog::CDP,        "CDP"       },
    {BCLog::WALLET,     "WALLET"    },
    {BCLog::LIBEVENT,   "LIBEVENT"  },
    {BCLog::DEX,        "DEX"       },
    {BCLog::RPCCMD,     "RPCCMD"    },
    {BCLog::DELEGATE,   "DELEGATE"  },
    {BCLog::PBFT,       "PBFT"      },
    {BCLog::BENCHMARK,  "BENCHMARK" },
    {BCLog::ALL,        "ALL"       },
};

static const std::string LOG_CATEGORY_UNKOWN = "";

const std::string& GetLogCategoryName(const BCLog::LogFlags& flag) {
    auto it = LOG_CATEGORY_MAP.find(flag);
    if (it != LOG_CATEGORY_MAP.end())
        return it->second;
    return LOG_CATEGORY_UNKOWN;
}

bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str)
{
    if (str == "") {
        flag = BCLog::ALL;
        return true;
    }
    const std::string &category = StrToUpper(str);
    for (auto& categoryPair : LOG_CATEGORY_MAP) {
        if (categoryPair.second == category) {
            flag = categoryPair.first;
            return true;
        }
    }
    return false;
}

std::string ListLogCategories()
{
    std::string ret;
    int outcount = 0;
    for (auto& categoryPair : LOG_CATEGORY_MAP) {
        // Omit the special cases.
        if (categoryPair.first != BCLog::NONE && categoryPair.first != BCLog::ALL) {
            if (outcount != 0) ret += ", ";
            ret += categoryPair.second;
            outcount++;
        }
    }
    return ret;
}

std::vector<CLogCategoryActive> ListActiveLogCategories()
{
    std::vector<CLogCategoryActive> ret;
    for (auto& categoryPair : LOG_CATEGORY_MAP) {
        // Omit the special cases.
        if (categoryPair.first != BCLog::NONE && categoryPair.first != BCLog::ALL) {
            CLogCategoryActive catActive;
            catActive.category = categoryPair.second;
            catActive.active = LogAcceptCategory(categoryPair.first);
            ret.push_back(catActive);
        }
    }
    return ret;
}

std::string BCLog::Logger::LogTimestampStr(const std::string& str)
{
    std::string strStamped;

    if (!m_log_timestamps)
        return str;

    if (m_started_new_line) {
        int64_t nTimeMicros = GetTimeMicros();
        strStamped = FormatISO8601DateTime(nTimeMicros/1000000);
        if (m_log_time_micros) {
            strStamped.pop_back();
            strStamped += strprintf(".%06dZ", nTimeMicros%1000000);
        }
        int64_t mocktime = GetMockTime();
        if (mocktime) {
            strStamped += " (mocktime: " + FormatISO8601DateTime(mocktime) + ")";
        }
        strStamped += ' ' + str;
    } else
        strStamped = str;

    return strStamped;
}

namespace BCLog {
    /** Belts and suspenders: make sure outgoing log messages don't contain
     * potentially suspicious characters, such as terminal control codes.
     *
     * This escapes control characters except newline ('\n') in C syntax.
     * It escapes instead of removes them to still allow for troubleshooting
     * issues where they accidentally end up in strings.
     */
    std::string LogEscapeMessage(const std::string& str) {
        std::string ret;
        for (char ch_in : str) {
            uint8_t ch = (uint8_t)ch_in;
            if ((ch >= 32 || ch == '\n') && ch != '\x7f') {
                ret += ch_in;
            } else {
                ret += strprintf("\\x%02x", ch);
            }
        }
        return ret;
    }

}

void BCLog::Logger::LogPrintStr(const BCLog::LogFlags& category, const char* file, int line, const char* func, const std::string& str) {

    std::lock_guard<std::mutex> scoped_lock(m_cs);
    std::string str_prefixed = LogEscapeMessage(str);

    string s_file = string(file);
    string s_func = string(func);

    if (m_print_file_line) {
        string file_line = strprintf("%s:%d", s_file, line);

        #ifndef VER_DEBUG
        if (file_line.size() > 38)
            file_line = strprintf("%s**%s", file_line.substr(0, 10), file_line.substr(file_line.size()-10, 10));

        if (s_func.size() > 20)
            s_func = s_func.substr(0, 20);
        #endif

        string file_line_func = strprintf("[%s %s]", file_line, s_func);
        string log_cat = strprintf("[%s]", GetLogCategoryName(category));
        string file_line_cat_func = strprintf("%-15s %-44s", log_cat, file_line_func);

        str_prefixed.insert(0, tfm::format("%-80s ", file_line_cat_func));
    }

    if (m_log_threadnames && m_started_new_line) {
        str_prefixed.insert(0, "[" + util::ThreadGetInternalName() + "] ");
    }

    str_prefixed = LogTimestampStr(str_prefixed);

    m_started_new_line = !str.empty() && str[str.size()-1] == '\n';

    if (m_buffering) {
        // buffer if we haven't started logging yet
        m_msgs_before_open.push_back(str_prefixed);
        return;
    }

    if (m_print_to_console) {
        // print to console
        fwrite(str_prefixed.data(), 1, str_prefixed.size(), stdout);
        fflush(stdout);
    }
    for (const auto& cb : m_print_callbacks) {
        cb(str_prefixed);
    }
    if (m_print_to_file) {

        assert(m_fileout != nullptr);
        // reopen the log file, if requested
        if (m_reopen_file) {
            m_reopen_file = false;
            FILE* new_fileout = fsbridge::fopen(m_file_path, "a");
            if (new_fileout) {
                setbuf(new_fileout, nullptr); // unbuffered
                    fclose(m_fileout);
                m_fileout = new_fileout;
            }
        }

        FileWriteStr(str_prefixed, m_fileout);
        m_totoal_written_size += str_prefixed.size();

        if(m_totoal_written_size > m_max_log_size){
            ShrinkDebugFile();
            m_totoal_written_size = 0;

        }

    }
}

void BCLog::Logger::ShrinkDebugFile()
{
    assert(!m_file_path.empty());
    string new_path =  strprintf("%s%s",m_file_path.string(), ".1");
    remove(new_path.c_str());
    rename(m_file_path.string().c_str(), new_path.c_str());
    m_reopen_file = true;

}