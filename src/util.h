// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_UTIL_H
#define COIN_UTIL_H

#if defined(HAVE_CONFIG_H)
#include "config/coin-config.h"
#endif

#include "commons/serialize.h"
#include "commons/tinyformat.h"
#include "compat/compat.h"

#include <stdarg.h>
#include <stdint.h>
#include <cstdio>
#include <exception>
#include <map>
#include <string>
#include <utility>
#include <vector>

#ifndef WIN32
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

#include <boost/filesystem/path.hpp>
#include <boost/thread.hpp>

class CNetAddr;
class uint256;

static const int64_t COIN = 100000000;
static const int64_t CENT = 1000000;

#define BEGIN(a) ((char*)&(a))
#define END(a) ((char*)&((&(a))[1]))
#define UBEGIN(a) ((unsigned char*)&(a))
#define UEND(a) ((unsigned char*)&((&(a))[1]))
#define ARRAYLEN(array) (sizeof(array) / sizeof((array)[0]))

// This is needed because the foreach macro can't get over the comma in pair<t1, t2>
#define PAIRTYPE(t1, t2) pair<t1, t2>

// Align by increasing pointer, must have extra space at end of buffer
template <size_t nBytes, typename T>
T* alignup(T* p) {
    union {
        T* ptr;
        size_t n;
    } u;
    u.ptr = p;
    u.n   = (u.n + (nBytes - 1)) & ~(nBytes - 1);
    return u.ptr;
}

#ifdef WIN32
#define MSG_DONTWAIT 0

#ifndef S_IRUSR
#define S_IRUSR 0400
#define S_IWUSR 0200
#endif
#else
#define MAX_PATH 1024
#endif
// As Solaris does not have the MSG_NOSIGNAL flag for send(2) syscall, it is defined as 0
#if !defined(HAVE_MSG_NOSIGNAL) && !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

inline void MilliSleep(int64_t n) {
// Boost's sleep_for was uninterruptable when backed by nanosleep from 1.50
// until fixed in 1.52. Use the deprecated sleep method for the broken case.
// See: https://svn.boost.org/trac/boost/ticket/7238
#if defined(HAVE_WORKING_BOOST_SLEEP_FOR)
    boost::this_thread::sleep_for(boost::chrono::milliseconds(n));
#elif defined(HAVE_WORKING_BOOST_SLEEP)
    boost::this_thread::sleep(boost::posix_time::milliseconds(n));
#else
// should never get here
#error missing boost sleep implementation
#endif
}

struct DebugLogFile {
    DebugLogFile() : m_newLine(true), m_fileout(NULL), m_mutexDebugLog(NULL) {}
    ~DebugLogFile() {
        if (m_fileout) {
            fclose(m_fileout);
            m_fileout = NULL;
        }
        if (m_mutexDebugLog) {
            delete m_mutexDebugLog;
            m_mutexDebugLog = NULL;
        }
    }
    bool m_newLine;
    FILE* m_fileout;
    boost::mutex* m_mutexDebugLog;
};

typedef map<string, DebugLogFile>::iterator DebugLogFileIt;

extern string strMiscWarning;
extern bool fNoListen;
extern volatile bool fReopenDebugLog;

void RandAddSeed();
void RandAddSeedPerfmon();
void SetupEnvironment();

// bool GetBoolArg(const string& strArg, bool fDefault);

// Get the LogFile iterator by category
bool FindLogFile(const char* category, DebugLogFileIt& logFileIt);
/* Return true if log accepts specified category */
bool LogAcceptCategory(const char* category);
/* Send a string to the log output */
int LogPrintStr(const string& str);
extern string GetLogHead(int line, const char* file, const char* category);
int LogPrintStr(const char* category, const string& str);

int LogPrintStr(const std::string& logName, DebugLogFile& logFile, const string& str);

#define strprintf tfm::format

#define ERRORMSG(...) error2(__LINE__, __FILE__, __VA_ARGS__)

#define LogPrint(tag, ...)                                                                           \
    {                                                                                                \
        DebugLogFileIt __logFileIt;                                                                  \
        if (FindLogFile(tag, __logFileIt)) {                                                         \
            LogTrace(tag, __logFileIt->first, __logFileIt->second, __LINE__, __FILE__, __VA_ARGS__); \
        }                                                                                            \
    }

#define MAKE_ERROR_AND_TRACE_FUNC(n)                                                                                 \
    /*   Print to debug.log if -debug=category switch is given OR category is NULL. */                               \
    template <TINYFORMAT_ARGTYPES(n)>                                                                                \
    static inline int LogTrace(const char* category, const std::string& logName, DebugLogFile& logFile, int line,    \
                               const char* file, const char* format, TINYFORMAT_VARARGS(n)) {                        \
        return LogPrintStr(logName, logFile,                                                                         \
                           GetLogHead(line, file, category) + tfm::format(format, TINYFORMAT_PASSARGS(n)));          \
    }                                                                                                                \
    /*   Log error and return false */                                                                               \
    template <TINYFORMAT_ARGTYPES(n)>                                                                                \
    static inline bool error2(int line, const char* file, const char* format1, TINYFORMAT_VARARGS(n)) {              \
        LogPrintStr("ERROR", GetLogHead(line, file, "ERROR") + tfm::format(format1, TINYFORMAT_PASSARGS(n)) + "\n"); \
        return false;                                                                                                \
    }

TINYFORMAT_FOREACH_ARGNUM(MAKE_ERROR_AND_TRACE_FUNC)

static inline bool error2(int line, const char* file, const char* format) {
    //	LogPrintStr(tfm::format("[%s:%d]: ", file, line)+string("ERROR: ") + format + "\n");
    LogPrintStr("ERROR", GetLogHead(line, file, "ERROR") + format + "\n");
    return false;
}

extern string GetLogHead(int line, const char* file, const char* category);

static inline int LogTrace(const char* category, const std::string& logName, DebugLogFile& logFile, int line,
                           const char* file, const char* format) {
    return LogPrintStr(logName, logFile, GetLogHead(line, file, category) + format);
}

// static inline bool errorTrace(const char* format) {
//	LogPrintStr(string("ERROR: ") + format + "\n");
//	return false;
//}

void LogException(std::exception* pex, const char* pszThread);
void PrintExceptionContinue(std::exception* pex, const char* pszThread);
string FormatMoney(int64_t n, bool fPlus = false);
bool ParseMoney(const string& str, int64_t& nRet);
bool ParseMoney(const char* pszIn, int64_t& nRet);
string SanitizeString(const string& str);
vector<unsigned char> ParseHex(const char* psz);
vector<unsigned char> ParseHex(const string& str);
bool IsHex(const string& str);
vector<unsigned char> DecodeBase64(const char* p, bool* pfInvalid = NULL);
string DecodeBase64(const string& str);
string EncodeBase64(const unsigned char* pch, size_t len);
string EncodeBase64(const string& str);
vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid = NULL);
string DecodeBase32(const string& str);
string EncodeBase32(const unsigned char* pch, size_t len);
string EncodeBase32(const string& str);
bool WildcardMatch(const char* psz, const char* mask);
bool WildcardMatch(const string& str, const string& mask);
void FileCommit(FILE* fileout);
bool TruncateFile(FILE* file, unsigned int length);
int RaiseFileDescriptorLimit(int nMinFD);
void AllocateFileRange(FILE* file, unsigned int offset, unsigned int length);
bool RenameOver(boost::filesystem::path src, boost::filesystem::path dest);
bool TryCreateDirectory(const boost::filesystem::path& p);
boost::filesystem::path GetDefaultDataDir();
const boost::filesystem::path& GetDataDir(bool fNetSpecific = true);
boost::filesystem::path GetConfigFile();
boost::filesystem::path GetAbsolutePath(const string& path);
boost::filesystem::path GetPidFile();
#ifndef WIN32
void CreatePidFile(const boost::filesystem::path& path, pid_t pid);
#endif
void ReadConfigFile(map<string, string>& mapSettingsRet, map<string, vector<string> >& mapMultiSettingsRet);
#ifdef WIN32
boost::filesystem::path GetSpecialFolderPath(int nFolder, bool fCreate = true);
#endif
boost::filesystem::path GetTempPath();
void ShrinkDebugFile();
int GetRandInt(int nMax);
uint64_t GetRand(uint64_t nMax);
uint256 GetRandHash();
int64_t GetTime();
void SetMockTime(int64_t nMockTimeIn);
int64_t GetAdjustedTime();
int64_t GetTimeOffset();
string FormatFullVersion();
string FormatSubVersion(const string& name, int nClientVersion, const vector<string>& comments);
void StringReplace(string& strBase, string strSrc, string strDes);
void AddTimeData(const CNetAddr& ip, int64_t nTime);
void runCommand(string strCommand);

inline string i64tostr(int64_t n) { return strprintf("%d", n); }

inline string itostr(int n) { return strprintf("%d", n); }

inline int64_t atoi64(const char* psz) {
#ifdef _MSC_VER
    return _atoi64(psz);
#else
    return strtoll(psz, NULL, 10);
#endif
}

inline int64_t atoi64(const string& str) {
#ifdef _MSC_VER
    return _atoi64(str.c_str());
#else
    return strtoll(str.c_str(), NULL, 10);
#endif
}

inline int atoi(const string& str) { return atoi(str.c_str()); }

inline int roundint(double d) { return (int)(d > 0 ? d + 0.5 : d - 0.5); }

inline int64_t roundint64(double d) { return (int64_t)(d > 0 ? d + 0.5 : d - 0.5); }

inline int64_t abs64(int64_t n) { return (n >= 0 ? n : -n); }

template <typename T>
string HexStr(const T itbegin, const T itend, bool fSpaces = false) {
    string rv;
    static const char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    rv.reserve((itend - itbegin) * 3);
    for (T it = itbegin; it < itend; ++it) {
        unsigned char val = (unsigned char)(*it);
        if (fSpaces && it != itbegin) rv.push_back(' ');
        rv.push_back(hexmap[val >> 4]);
        rv.push_back(hexmap[val & 15]);
    }

    return rv;
}

template <typename T>
inline string HexStr(const T& vch, bool fSpaces = false) {
    return HexStr(vch.begin(), vch.end(), fSpaces);
}

inline int64_t GetTimeMillis() {
    return (boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) -
            boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1)))
        .total_milliseconds();
}

inline int64_t GetTimeMicros() {
    return (boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) -
            boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1)))
        .total_microseconds();
}

string DateTimeStrFormat(const char* pszFormat, int64_t nTime);

template <typename T>
void skipspaces(T& it) {
    while (isspace(*it)) ++it;
}

inline bool IsSwitchChar(char c) {
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

/**
 * Timing-attack-resistant comparison.
 * Takes time proportional to length
 * of first argument.
 */
template <typename T>
bool TimingResistantEqual(const T& a, const T& b) {
    if (b.size() == 0) return a.size() == 0;

    size_t accumulator = a.size() ^ b.size();
    for (size_t i = 0; i < a.size(); i++) accumulator |= a[i] ^ b[i % b.size()];

    return accumulator == 0;
}

/** Median filter over a stream of values.
 * Returns the median of the last N numbers
 */
template <typename T>
class CMedianFilter {
private:
    vector<T> vValues;
    vector<T> vSorted;
    unsigned int nSize;

public:
    CMedianFilter(unsigned int size, T initial_value) : nSize(size) {
        vValues.reserve(size);
        vValues.push_back(initial_value);
        vSorted = vValues;
    }

    void input(T value) {
        if (vValues.size() == nSize) {
            vValues.erase(vValues.begin());
        }
        vValues.push_back(value);

        vSorted.resize(vValues.size());
        copy(vValues.begin(), vValues.end(), vSorted.begin());
        sort(vSorted.begin(), vSorted.end());
    }

    T median() const {
        int size = vSorted.size();
        assert(size > 0);
        if (size & 1)  // Odd number of elements
        {
            return vSorted[size / 2];
        } else  // Even number of elements
        {
            return (vSorted[size / 2 - 1] + vSorted[size / 2]) / 2;
        }
    }

    int size() const { return vValues.size(); }

    vector<T> sorted() const { return vSorted; }
};

#ifdef WIN32
inline void SetThreadPriority(int nPriority) { SetThreadPriority(GetCurrentThread(), nPriority); }
#else

// PRIO_MAX is not defined on Solaris
#ifndef PRIO_MAX
#define PRIO_MAX 20
#endif
#define THREAD_PRIORITY_LOWEST PRIO_MAX
#define THREAD_PRIORITY_BELOW_NORMAL 2
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL (-2)

inline void SetThreadPriority(int nPriority) {
    // It's unclear if it's even possible to change thread priorities on Linux,
    // but we really and truly need it for the generation threads.
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, nPriority);
#else
    setpriority(PRIO_PROCESS, 0, nPriority);
#endif
}
#endif

void RenameThread(const char* name);

inline uint32_t ByteReverse(uint32_t value) {
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    return (value << 16) | (value >> 16);
}

// Standard wrapper for do-something-forever thread functions.
// "Forever" really means until the thread is interrupted.
// Use it like:
//   new boost::thread(boost::bind(&LoopForever<void (*)()>, "dumpaddr", &DumpAddresses, 900000));
// or maybe:
//    boost::function<void()> f = boost::bind(&FunctionWithArg, argument);
//    threadGroup.create_thread(boost::bind(&LoopForever<boost::function<void()> >, "nothing", f, milliseconds));
template <typename Callable>
void LoopForever(const char* name, Callable func, int64_t msecs) {
    string s = strprintf("coin-%s", name);
    RenameThread(s.c_str());
    LogPrint("INFO", "%s thread start\n", name);
    try {
        while (1) {
            MilliSleep(msecs);
            func();
        }
    } catch (boost::thread_interrupted) {
        LogPrint("INFO", "%s thread stop\n", name);
        throw;
    } catch (std::exception& e) {
        PrintExceptionContinue(&e, name);
        throw;
    } catch (...) {
        PrintExceptionContinue(NULL, name);
        throw;
    }
}
// .. and a wrapper that just calls func once
template <typename Callable>
void TraceThread(const char* name, Callable func) {
    string s = strprintf("coin-%s", name);
    RenameThread(s.c_str());
    try {
        LogPrint("INFO", "%s thread start\n", name);
        func();
        LogPrint("INFO", "%s thread exit\n", name);
    } catch (boost::thread_interrupted) {
        LogPrint("INFO", "%s thread interrupt\n", name);
        throw;
    } catch (std::exception& e) {
        PrintExceptionContinue(&e, name);
        throw;
    } catch (...) {
        PrintExceptionContinue(NULL, name);
        throw;
    }
}

/**
 * Tests if the given character is a whitespace character. The whitespace characters
 * are: space, form-feed ('\f'), newline ('\n'), carriage return ('\r'), horizontal
 * tab ('\t'), and vertical tab ('\v').
 *
 * This function is locale independent. Under the C locale this function gives the
 * same result as std::isspace.
 *
 * @param[in] c     character to test
 * @return          true if the argument is a whitespace character; otherwise false
 */
constexpr inline bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

/**
 * Convert string to signed 32-bit integer with strict parse error feedback.
 * @returns true if the entire string could be parsed as valid integer,
 *   false if not the entire string could be parsed or when overflow or underflow occurred.
 */
bool ParseInt32(const std::string& str, int32_t* out);

inline string _(const char* str) { return string(str); }

#endif