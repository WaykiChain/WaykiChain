// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include "fs.h"
#include "commons/tinyformat.h"
#include <boost/filesystem.hpp>
#include <atomic>
#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <vector>

namespace fs = boost::filesystem;

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fLogIPs;

struct CLogCategoryActive
{
    std::string category;
    bool active;
};

namespace BCLog { //blockchain log
    enum LogFlags : uint32_t {
        NONE        = 0,
        INFO        = (1 <<  0),
        ERROR       = (1 <<  1),
        DEBUG       = (1 <<  2),
        NET         = (1 <<  3),
        MINER       = (1 <<  4),
        ALERT       = (1 <<  5),
        CDB         = (1 <<  6),
        BDB         = (1 <<  7),
        LDB         = (1 <<  8),
        LUAVM       = (1 <<  9),
        WASM        = (1 << 10),
        LOCK        = (1 << 11),
        HTTP        = (1 << 12),
        RPC         = (1 << 13),
        REINDEX     = (1 << 14),
        PROFIT      = (1 << 15),
        ADDRMAN     = (1 << 16),
        PRICEFEED   = (1 << 17),
        CDP         = (1 << 18),
        WALLET      = (1 << 19),
        LIBEVENT    = (1 << 20),
        DEX         = (1 << 21),
        RPCCMD      = (1 << 22),
        DELEGATE    = (1 << 23),
        PBFT_LOG    = (1 << 24),
        ALL         = ~(uint32_t)0,
    };

    class Logger
    {
    private:
        mutable std::mutex m_cs;                   // Can not use Mutex from sync.h because in debug mode it would cause a deadlock when a potential deadlock was detected
        FILE* m_fileout = nullptr;                 // GUARDED_BY(m_cs)
        std::list<std::string> m_msgs_before_open; // GUARDED_BY(m_cs)
        bool m_buffering{true};                    //!< Buffer messages before logging can be started. GUARDED_BY(m_cs)

        /**
         * m_started_new_line is a state variable that will suppress printing of
         * the timestamp when multiple calls are made that don't end in a
         * newline.
         */
        std::atomic_bool m_started_new_line{true};

        /** Log categories bitfield. */
        std::atomic<uint32_t> m_categories{0};

        std::string LogTimestampStr(const std::string& str);

        /** Slots that connect to the print signal */
        std::list<std::function<void(const std::string&)>> m_print_callbacks /* GUARDED_BY(m_cs) */ {};

    public:

        bool m_print_to_console = false;
        bool m_print_to_file = false;
        bool m_print_file_line = false;

        bool m_log_timestamps = DEFAULT_LOGTIMESTAMPS;
        bool m_log_time_micros = DEFAULT_LOGTIMEMICROS;
        bool m_log_threadnames = DEFAULT_LOGTHREADNAMES;
        uint64_t m_totoal_written_size = 0 ;
        uint64_t m_max_log_size = 0 ;

        fs::path m_file_path;
        std::atomic<bool> m_reopen_file{false};

        /** Send a string to the log output */
        void LogPrintStr(const BCLog::LogFlags& category, const char* file, int line,
            const std::string& str);

        /** Returns whether logs will be written to any output */
        bool Enabled() const
        {
            std::lock_guard<std::mutex> scoped_lock(m_cs);
            return m_buffering || m_print_to_console || m_print_to_file || !m_print_callbacks.empty();
        }

        /** Connect a slot to the print signal and return the connection */
        std::list<std::function<void(const std::string&)>>::iterator PushBackCallback(std::function<void(const std::string&)> fun)
        {
            std::lock_guard<std::mutex> scoped_lock(m_cs);
            m_print_callbacks.push_back(std::move(fun));
            return --m_print_callbacks.end();
        }

        /** Delete a connection */
        void DeleteCallback(std::list<std::function<void(const std::string&)>>::iterator it)
        {
            std::lock_guard<std::mutex> scoped_lock(m_cs);
            m_print_callbacks.erase(it);
        }

        uint64_t GetCurrentLogSize() ;

        /** Start logging (and flush all buffered messages) */
        bool StartLogging();
        /** make sure the logs are saved to the dest storage */
        void Flush();
        /** Only for testing */
        void DisconnectTestLogger();

        void ShrinkDebugFile();

        uint32_t GetCategoryMask() const { return m_categories.load(); }

        void EnableCategory(LogFlags flag);
        bool EnableCategory(const std::string& str);
        void DisableCategory(LogFlags flag);
        bool DisableCategory(const std::string& str);

        bool WillLogCategory(LogFlags category) const;

        bool DefaultShrinkDebugFile() const;
    };

} // namespace BCLog

BCLog::Logger& LogInstance();

/** Return true if log accepts specified category */
static inline bool LogAcceptCategory(BCLog::LogFlags category)
{
    return LogInstance().WillLogCategory(category);
}

/** Returns a string with the log categories. */
std::string ListLogCategories();

/** Returns a vector of the active log categories. */
std::vector<CLogCategoryActive> ListActiveLogCategories();

/** Return true if str parses as a log category and set the flag */
bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str);

// Be conservative when using LogPrintf/error or other things which
// unconditionally log to debug.log! It should not be the case that an inbound
// peer can fill up a user's disk with debug.log entries.

template <typename... Args>
static inline void LogPrintf(const BCLog::LogFlags& category, const char* file, int line,
    bool lineFeed, const char* fmt, const Args&... args) {

    if (LogInstance().Enabled()) {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
            if (lineFeed) log_msg += "\n";
        } catch (tinyformat::format_error& fmterr) {
            /* Original format string will have newline so don't add one here */
            log_msg = "format ERROR \"" + std::string(fmterr.what()) + "\" while formatting log message: " + fmt + "\n";
        }
        LogInstance().LogPrintStr(category, file, line, log_msg);
    }
}

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.
#define LogPrint(category, ...)                                                                    \
    {                                                                                              \
        if (LogAcceptCategory((category))) {                                                       \
            LogPrintf(category, __FILE__, __LINE__, false, __VA_ARGS__);                           \
        }                                                                                          \
    }

/*   Log error and return false */
template <typename... Args>
static inline bool LogError(const char *file, int line, const char *fmt,
                            const Args &... args) {
    LogPrintf(BCLog::ERROR, file, line, true, fmt, args...);
    return false;
}

#define ERRORMSG(...) LogError(__FILE__, __LINE__, __VA_ARGS__)

#endif // BITCOIN_LOGGING_H