// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "util.h"
#include "chainparams.h"
#include "configuration.h"
#include "netbase.h"
#include "sync.h"
#include "commons/uint256.h"
#include "version.h"

#include <stdarg.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#ifndef WIN32
// for posix_fallocate
#ifdef __linux_

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#define _POSIX_C_SOURCE 200112L
#include <sys/prctl.h>

#endif

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <algorithm>

#else

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#pragma warning(disable : 4804)
#pragma warning(disable : 4805)
#pragma warning(disable : 4717)
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501

#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501

#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <io.h> /* for _commit */
#include <shlobj.h>
#endif

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <boost/algorithm/string/case_conv.hpp>  // for to_lower()
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>  // for startswith() and endswith()
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>
// Work around clang compilation problem in Boost 1.46:
// /usr/include/boost/program_options/detail/config_file.hpp:163:17: error: call to function
// 'to_internal' that is neither visible in the template definition nor found by argument-dependent
// lookup See also:
// http://stackoverflow.com/questions/10020179/compilation-fail-in-boost-librairies-program-options
//           http://clang.debian.net/status.php?version=3.0&key=CANNOT_FIND_FUNCTION
namespace boost {
namespace program_options {
string to_internal(const string&);
}
}  // namespace boost

using namespace std;

bool fDaemon = false;
string strMiscWarning;
bool fNoListen                = false;
volatile bool fReopenDebugLog = false;

// Init OpenSSL library multithreading support
static CCriticalSection** ppmutexOpenSSL;
void locking_callback(int mode, int i, const char* file, int line) {
    if (mode & CRYPTO_LOCK) {
        ENTER_CRITICAL_SECTION(*ppmutexOpenSSL[i]);
    } else {
        LEAVE_CRITICAL_SECTION(*ppmutexOpenSSL[i]);
    }
}

// Init
class CInit {
public:
    CInit() {
        // Init OpenSSL library multithreading support
        ppmutexOpenSSL =
            (CCriticalSection**)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(CCriticalSection*));
        for (int i = 0; i < CRYPTO_num_locks(); i++) ppmutexOpenSSL[i] = new CCriticalSection();
        CRYPTO_set_locking_callback(locking_callback);

#ifdef WIN32
        // Seed random number generator with screen scrape and other hardware sources
        RAND_screen();
#endif

        // Seed random number generator with performance counter
        RandAddSeed();
    }
    ~CInit() {
        // Shutdown OpenSSL library multithreading support
        CRYPTO_set_locking_callback(NULL);
        for (int i = 0; i < CRYPTO_num_locks(); i++) delete ppmutexOpenSSL[i];
        OPENSSL_free(ppmutexOpenSSL);
    }
} instance_of_cinit;

// void RandAddSeed() {
//// Seed with CPU performance counter
// int64_t nCounter = GetPerformanceCounter();
// RAND_add(&nCounter, sizeof(nCounter), 1.5);
// memset(&nCounter, 0, sizeof(nCounter));
//}

// void RandAddSeedPerfmon() {
// RandAddSeed();
//
//// This can take up to 2 seconds, so only do it every 10 minutes
// static int64_t nLastPerfmon;
// if (GetTime() < nLastPerfmon + 10 * 60)
// return;
// nLastPerfmon = GetTime();
//
//#ifdef WIN32
//// Don't need this on Linux, OpenSSL automatically uses /dev/urandom
//// Seed with the entire set of perfmon data
// unsigned char pdata[250000];
// memset(pdata, 0, sizeof(pdata));
// unsigned long nSize = sizeof(pdata);
// long ret = RegQueryValueExA(HKEY_PERFORMANCE_DATA, "Global", NULL, NULL, pdata, &nSize);
// RegCloseKey(HKEY_PERFORMANCE_DATA);
// if (ret == ERROR_SUCCESS) {
// RAND_add(pdata, nSize, nSize / 100.0);
// OPENSSL_cleanse(pdata, nSize);
// LogPrint("rand", "RandAddSeed() %lu bytes\n", nSize);
//}
//#endif
//}

// uint64_t GetRand(uint64_t nMax) {
// if (nMax == 0)
// return 0;
//
//// The range of the random source must be a multiple of the modulus
//// to give every possible output value an equal possibility
// uint64_t nRange = (numeric_limits<uint64_t>::max() / nMax) * nMax;
// uint64_t nRand = 0;
// do
// RAND_bytes((unsigned char*) &nRand, sizeof(nRand));
// while (nRand >= nRange);
// return (nRand % nMax);
//}

// int GetRandInt(int nMax) {
// return GetRand(nMax);
//}

// uint256 GetRandHash() {
// uint256 hash;
// RAND_bytes((unsigned char*) &hash, sizeof(hash));
// return hash;
//}

// LogPrint("INFO",) has been broken a couple of times now
// by well-meaning people adding mutexes in the most straightforward way.
// It breaks because it may be called by global destructors during shutdown.
// Since the order of destruction of static/global objects is undefined,
// defining a mutex as a global object doesn't work (the mutex gets
// destroyed, and then some later destructor calls OutputDebugStringF,
// maybe indirectly, and you get a core dump at shutdown trying to lock
// the mutex).

static boost::once_flag debugPrintInitFlag = BOOST_ONCE_INIT;
// We use boost::call_once() to make sure these are initialized in
// in a thread-safe manner the first time it is called:
// static FILE* fileout = NULL;
// static boost::mutex* mutexDebugLog = NULL;

static map<string, DebugLogFile> g_DebugLogs;

static void DebugPrintInit() {
    shared_ptr<vector<string>> te    = SysCfg().GetMultiArgsMap("-debug");
    const vector<string>& categories = *(te.get());
    set<string> logfiles(categories.begin(), categories.end());

    shared_ptr<vector<string>> tmp     = SysCfg().GetMultiArgsMap("-nodebug");
    const vector<string>& nocategories = *(tmp.get());
    set<string> nologfiles(nocategories.begin(), nocategories.end());

    if (SysCfg().IsDebugAll()) {
        logfiles.clear();
        logfiles = nologfiles;
        logfiles.insert("debug");

        for (auto& tmp : logfiles) {
            g_DebugLogs[tmp];
        }

        {
            FILE* fileout = NULL;
            boost::filesystem::path pathDebug;
            string file = "debug.log";
            pathDebug   = GetDataDir() / file;
            fileout     = fopen(pathDebug.string().c_str(), "a");
            if (fileout) {
                DebugLogFile& log = g_DebugLogs["debug"];
                setbuf(fileout, NULL);  // unbuffered
                log.m_fileout       = fileout;
                log.m_mutexDebugLog = new boost::mutex();
            }
        }

    } else {
        for (auto& tmp : nologfiles) {
            logfiles.erase(tmp);
        }
        logfiles.insert("debug");

        for (auto& cat : logfiles) {
            FILE* fileout = NULL;
            boost::filesystem::path pathDebug;
            string file = cat + ".log";
            pathDebug   = GetDataDir() / file;
            fileout     = fopen(pathDebug.string().c_str(), "a");
            if (fileout) {
                DebugLogFile& log = g_DebugLogs[cat];
                setbuf(fileout, NULL);  // unbuffered
                log.m_fileout       = fileout;
                log.m_mutexDebugLog = new boost::mutex();
            }
        }
    }
}

int LogPrintStr(const string& str) { return LogPrintStr(NULL, str); }

string GetLogHead(int line, const char* file, const char* category) {
    string te(category != NULL ? category : "");
    if (SysCfg().IsDebug()) {
        if (SysCfg().IsLogPrintLine()) return tfm::format("[%s:%d]%s: ", file, line, te);
    }
    return string("");
}
/**
 *  日志文件预处理。写日志文件前被调用，检测文件A是否超长
 *  当超长则先将原文件A重命名为Abak，再打开并创建A文件，删除重命名文件Abak，返回。
 * @param path  	文件路径
 * @param len  		写入数据的长度
 * @param stream  	文件的句柄
 * @return
 */
int LogFilePreProcess(const char* path, size_t len, FILE** stream) {
    if ((NULL == path) || (len <= 0) || (NULL == *stream)) {
        //    	assert(0);
        return -1;
    }
    int lSize = ftell(*stream);                            // 当前文件长度
    if (lSize + len > (size_t)SysCfg().GetLogMaxSize()) {  // 文件超长，关闭，删除，再创建
        FILE* fileout = NULL;
        //      cout<<"file name:" << path <<"free point:"<< static_cast<const void*>(*stream)<<
        //      "lSize: "<< lSize << "len: " << len<<endl;
        fclose(*stream);

        string bkFile = strprintf("%sbak", path);
        rename(path, bkFile.c_str());  // 原文件重命名
        fileout = fopen(path, "a+");   // 重新打开， 类似于删除文件.
        if (fileout) {
            //		    cout << "file new:" <<static_cast<const void*>(fileout) << endl;
            *stream = fileout;
            if (remove(bkFile.c_str()) != 0)  // 删除重命名文件
            {
                //				 assert(0);
                return -1;
            }
        } else {
            //         cout<<"LogFilePreProcess create new file err"<<endl;
            return -1;
        }
    }
    return 1;
}


bool FindLogFile(const char* category, DebugLogFileIt &logFileIt) {

    if (!SysCfg().IsDebug()) {
        logFileIt = g_DebugLogs.end();
        return false;
    }

    boost::call_once(&DebugPrintInit, debugPrintInitFlag);

    if (SysCfg().IsDebugAll()) {
        if (NULL != category) {
            logFileIt = g_DebugLogs.find(category);
            if (logFileIt != g_DebugLogs.end()) {
                return true;
            }
        }
        logFileIt = g_DebugLogs.find("debug");
        return logFileIt != g_DebugLogs.end();
    } else {
        logFileIt = g_DebugLogs.find((NULL == category) ? ("debug") : (category));
        return logFileIt != g_DebugLogs.end();
    }
}

int LogPrintStr(const std::string &logName, DebugLogFile &logFile, const string& str) {

    if (!SysCfg().IsDebug()) {
        return 0;
    }

    int ret = 0;  // Returns total number of characters written

    boost::call_once(&DebugPrintInit, debugPrintInitFlag);

    if (SysCfg().IsPrintLogToConsole()) {
        // print to console
        ret = fwrite(str.data(), 1, str.size(), stdout);
    }
    if (SysCfg().IsPrintLogToFile()) {
        DebugLogFile& log = logFile;
        boost::mutex::scoped_lock scoped_lock(*log.m_mutexDebugLog);

        boost::filesystem::path pathDebug;
        string file = logName + ".log";   //  it->first.c_str() = "INFO"
        pathDebug   = GetDataDir() / file;  // /home/share/bille/Coin_test/regtest/INFO.log
        // Debug print useful for profiling
        if (SysCfg().IsLogTimestamps() && log.m_newLine) {
            string timeFormat = DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime());
            LogFilePreProcess(pathDebug.string().c_str(), timeFormat.length() + 1 + str.size(),
                              &log.m_fileout);
            ret += fprintf(log.m_fileout, "%s ", timeFormat.c_str());
            //			ret += fprintf(log.m_fileout, "%s ", DateTimeStrFormat("%Y-%m-%d %H:%M:%S",
            //GetTime()).c_str());
        }
        if (!str.empty() && str[str.size() - 1] == '\n') {
            log.m_newLine = true;
        } else {
            log.m_newLine = false;
        }
        LogFilePreProcess(pathDebug.string().c_str(), str.size(), &log.m_fileout);
        ret = fwrite(str.data(), 1, str.size(), log.m_fileout);
    }
    return ret;
}

int LogPrintStr(const char* category, const string& str) {

    DebugLogFileIt it;
    if (!FindLogFile(category, it)) {
        return 0;
    }

    return LogPrintStr(it->first, it->second, str);
}

string FormatMoney(int64_t n, bool fPlus) {
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    int64_t n_abs     = (n > 0 ? n : -n);
    int64_t quotient  = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    string str        = strprintf("%d.%08d", quotient, remainder);

    // Right-trim excess zeros before the decimal point:
    int nTrim = 0;
    for (int i = str.size() - 1; (str[i] == '0' && isdigit(str[i - 2])); --i) ++nTrim;
    if (nTrim) str.erase(str.size() - nTrim, nTrim);

    if (n < 0)
        str.insert((unsigned int)0, 1, '-');
    else if (fPlus && n > 0)
        str.insert((unsigned int)0, 1, '+');
    return str;
}

bool ParseMoney(const string& str, int64_t& nRet) { return ParseMoney(str.c_str(), nRet); }

bool ParseMoney(const char* pszIn, int64_t& nRet) {
    string strWhole;
    int64_t nUnits = 0;
    const char* p  = pszIn;
    while (isspace(*p)) p++;
    for (; *p; p++) {
        if (*p == '.') {
            p++;
            int64_t nMult = CENT * 10;
            while (isdigit(*p) && (nMult > 0)) {
                nUnits += nMult * (*p++ - '0');
                nMult /= 10;
            }
            break;
        }
        if (isspace(*p)) break;
        if (!isdigit(*p)) return false;
        strWhole.insert(strWhole.end(), *p);
    }
    for (; *p; p++)
        if (!isspace(*p)) return false;
    if (strWhole.size() > 10)  // guard against 63 bit overflow
        return false;
    if (nUnits < 0 || nUnits > COIN) return false;
    int64_t nWhole = atoi64(strWhole);
    int64_t nValue = nWhole * COIN + nUnits;

    nRet = nValue;
    return true;
}

// safeChars chosen to allow simple messages/URLs/email addresses, but avoid anything
// even possibly remotely dangerous like & or >
static string safeChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890 .,;_/:?@");
string SanitizeString(const string& str) {
    string strResult;
    for (string::size_type i = 0; i < str.size(); i++) {
        if (safeChars.find(str[i]) != string::npos) strResult.push_back(str[i]);
    }
    return strResult;
}

const signed char p_util_hexdigit[256] = {
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  0,   1,   2,   3,  4,  5,  6,  7,  8,  9,   -1,  -1,
    -1,  -1,  -1,  -1, -1, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, 0xa, 0xb, 0xc,
    0xd, 0xe, 0xf, -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1, -1, -1,  -1,  -1,
    -1,  -1,  -1,  -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1, -1,
};

bool IsHex(const string& str) {
    for (char c : str) {
        if (HexDigit(c) < 0) return false;
    }
    return (str.size() > 0) && (str.size() % 2 == 0);
}

vector<unsigned char> ParseHex(const char* psz) {
    // convert hex dump to vector
    vector<unsigned char> vch;
    while (true) {
        while (isspace(*psz)) psz++;
        signed char c = HexDigit(*psz++);
        if (c == (signed char)-1) break;
        unsigned char n = (c << 4);
        c               = HexDigit(*psz++);
        if (c == (signed char)-1) break;
        n |= c;
        vch.push_back(n);
    }
    return vch;
}

vector<unsigned char> ParseHex(const string& str) { return ParseHex(str.c_str()); }

static void InterpretNegativeSetting(string name, map<string, string>& mapSettingsRet) {
    // interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
    if (name.find("-no") == 0) {
        string positive("-");
        positive.append(name.begin() + 3, name.end());
        if (mapSettingsRet.count(positive) == 0) {
            bool value               = !SysCfg().GetBoolArg(name, false);
            mapSettingsRet[positive] = (value ? "1" : "0");
        }
    }
}

string EncodeBase64(const unsigned char* pch, size_t len) {
    static const char* pbase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    string strRet = "";
    strRet.reserve((len + 2) / 3 * 4);

    int mode = 0, left = 0;
    const unsigned char* pchEnd = pch + len;

    while (pch < pchEnd) {
        int enc = *(pch++);
        switch (mode) {
            case 0:  // we have no bits
                strRet += pbase64[enc >> 2];
                left = (enc & 3) << 4;
                mode = 1;
                break;

            case 1:  // we have two bits
                strRet += pbase64[left | (enc >> 4)];
                left = (enc & 15) << 2;
                mode = 2;
                break;

            case 2:  // we have four bits
                strRet += pbase64[left | (enc >> 6)];
                strRet += pbase64[enc & 63];
                mode = 0;
                break;
        }
    }

    if (mode) {
        strRet += pbase64[left];
        strRet += '=';
        if (mode == 1) strRet += '=';
    }

    return strRet;
}

string EncodeBase64(const string& str) {
    return EncodeBase64((const unsigned char*)str.c_str(), str.size());
}

vector<unsigned char> DecodeBase64(const char* p, bool* pfInvalid) {
    static const int decode64_table[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62,
        -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0,
        1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    if (pfInvalid) *pfInvalid = false;

    vector<unsigned char> vchRet;
    vchRet.reserve(strlen(p) * 3 / 4);

    int mode = 0;
    int left = 0;

    while (1) {
        int dec = decode64_table[(unsigned char)*p];
        if (dec == -1) break;
        p++;
        switch (mode) {
            case 0:  // we have no bits and get 6
                left = dec;
                mode = 1;
                break;

            case 1:  // we have 6 bits and keep 4
                vchRet.push_back((left << 2) | (dec >> 4));
                left = dec & 15;
                mode = 2;
                break;

            case 2:  // we have 4 bits and get 6, we keep 2
                vchRet.push_back((left << 4) | (dec >> 2));
                left = dec & 3;
                mode = 3;
                break;

            case 3:  // we have 2 bits and get 6
                vchRet.push_back((left << 6) | dec);
                mode = 0;
                break;
        }
    }

    if (pfInvalid) switch (mode) {
            case 0:  // 4n base64 characters processed: ok
                break;

            case 1:  // 4n+1 base64 character processed: impossible
                *pfInvalid = true;
                break;

            case 2:  // 4n+2 base64 characters processed: require '=='
                if (left || p[0] != '=' || p[1] != '=' || decode64_table[(unsigned char)p[2]] != -1)
                    *pfInvalid = true;
                break;

            case 3:  // 4n+3 base64 characters processed: require '='
                if (left || p[0] != '=' || decode64_table[(unsigned char)p[1]] != -1)
                    *pfInvalid = true;
                break;
        }

    return vchRet;
}

string DecodeBase64(const string& str) {
    vector<unsigned char> vchRet = DecodeBase64(str.c_str());
    return string((const char*)&vchRet[0], vchRet.size());
}

string EncodeBase32(const unsigned char* pch, size_t len) {
    static const char* pbase32 = "abcdefghijklmnopqrstuvwxyz234567";

    string strRet = "";
    strRet.reserve((len + 4) / 5 * 8);

    int mode = 0, left = 0;
    const unsigned char* pchEnd = pch + len;

    while (pch < pchEnd) {
        int enc = *(pch++);
        switch (mode) {
            case 0:  // we have no bits
                strRet += pbase32[enc >> 3];
                left = (enc & 7) << 2;
                mode = 1;
                break;

            case 1:  // we have three bits
                strRet += pbase32[left | (enc >> 6)];
                strRet += pbase32[(enc >> 1) & 31];
                left = (enc & 1) << 4;
                mode = 2;
                break;

            case 2:  // we have one bit
                strRet += pbase32[left | (enc >> 4)];
                left = (enc & 15) << 1;
                mode = 3;
                break;

            case 3:  // we have four bits
                strRet += pbase32[left | (enc >> 7)];
                strRet += pbase32[(enc >> 2) & 31];
                left = (enc & 3) << 3;
                mode = 4;
                break;

            case 4:  // we have two bits
                strRet += pbase32[left | (enc >> 5)];
                strRet += pbase32[enc & 31];
                mode = 0;
        }
    }

    static const int nPadding[5] = {0, 6, 4, 3, 1};
    if (mode) {
        strRet += pbase32[left];
        for (int n = 0; n < nPadding[mode]; n++) strRet += '=';
    }

    return strRet;
}

string EncodeBase32(const string& str) {
    return EncodeBase32((const unsigned char*)str.c_str(), str.size());
}

vector<unsigned char> DecodeBase32(const char* p, bool* pfInvalid) {
    static const int decode32_table[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,
        1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
        23, 24, 25, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    if (pfInvalid) *pfInvalid = false;

    vector<unsigned char> vchRet;
    vchRet.reserve((strlen(p)) * 5 / 8);

    int mode = 0;
    int left = 0;

    while (1) {
        int dec = decode32_table[(unsigned char)*p];
        if (dec == -1) break;
        p++;
        switch (mode) {
            case 0:  // we have no bits and get 5
                left = dec;
                mode = 1;
                break;

            case 1:  // we have 5 bits and keep 2
                vchRet.push_back((left << 3) | (dec >> 2));
                left = dec & 3;
                mode = 2;
                break;

            case 2:  // we have 2 bits and keep 7
                left = left << 5 | dec;
                mode = 3;
                break;

            case 3:  // we have 7 bits and keep 4
                vchRet.push_back((left << 1) | (dec >> 4));
                left = dec & 15;
                mode = 4;
                break;

            case 4:  // we have 4 bits, and keep 1
                vchRet.push_back((left << 4) | (dec >> 1));
                left = dec & 1;
                mode = 5;
                break;

            case 5:  // we have 1 bit, and keep 6
                left = left << 5 | dec;
                mode = 6;
                break;

            case 6:  // we have 6 bits, and keep 3
                vchRet.push_back((left << 2) | (dec >> 3));
                left = dec & 7;
                mode = 7;
                break;

            case 7:  // we have 3 bits, and keep 0
                vchRet.push_back((left << 5) | dec);
                mode = 0;
                break;
        }
    }

    if (pfInvalid) switch (mode) {
            case 0:  // 8n base32 characters processed: ok
                break;

            case 1:  // 8n+1 base32 characters processed: impossible
            case 3:  //   +3
            case 6:  //   +6
                *pfInvalid = true;
                break;

            case 2:  // 8n+2 base32 characters processed: require '======'
                if (left || p[0] != '=' || p[1] != '=' || p[2] != '=' || p[3] != '=' ||
                    p[4] != '=' || p[5] != '=' || decode32_table[(unsigned char)p[6]] != -1)
                    *pfInvalid = true;
                break;

            case 4:  // 8n+4 base32 characters processed: require '===='
                if (left || p[0] != '=' || p[1] != '=' || p[2] != '=' || p[3] != '=' ||
                    decode32_table[(unsigned char)p[4]] != -1)
                    *pfInvalid = true;
                break;

            case 5:  // 8n+5 base32 characters processed: require '==='
                if (left || p[0] != '=' || p[1] != '=' || p[2] != '=' ||
                    decode32_table[(unsigned char)p[3]] != -1)
                    *pfInvalid = true;
                break;

            case 7:  // 8n+7 base32 characters processed: require '='
                if (left || p[0] != '=' || decode32_table[(unsigned char)p[1]] != -1)
                    *pfInvalid = true;
                break;
        }

    return vchRet;
}

string DecodeBase32(const string& str) {
    vector<unsigned char> vchRet = DecodeBase32(str.c_str());
    return string((const char*)&vchRet[0], vchRet.size());
}

bool WildcardMatch(const char* psz, const char* mask) {
    while (true) {
        switch (*mask) {
            case '\0':
                return (*psz == '\0');
            case '*':
                return WildcardMatch(psz, mask + 1) || (*psz && WildcardMatch(psz + 1, mask));
            case '?':
                if (*psz == '\0') return false;
                break;
            default:
                if (*psz != *mask) return false;
                break;
        }
        psz++;
        mask++;
    }
    return false;
}

bool WildcardMatch(const string& str, const string& mask) {
    return WildcardMatch(str.c_str(), mask.c_str());
}

static string FormatException(exception* pex, const char* pszThread) {
#ifdef WIN32
    char pszModule[MAX_PATH] = "";
    GetModuleFileNameA(NULL, pszModule, sizeof(pszModule));
#else
    const char* pszModule = "Coin";
#endif
    if (pex)
        return strprintf("EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(),
                         pex->what(), pszModule, pszThread);
    else
        return strprintf("UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
}

void LogException(exception* pex, const char* pszThread) {
    string message = FormatException(pex, pszThread);
    LogPrint("INFO", "\n%s", message);
}

void PrintExceptionContinue(exception* pex, const char* pszThread) {
    string message = FormatException(pex, pszThread);
    LogPrint("INFO", "\n\n************************\n%s\n", message);
    fprintf(stderr, "\n\n************************\n%s\n", message.c_str());
    strMiscWarning = message;
}

boost::filesystem::path GetDefaultDataDir() {
    namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\Coin
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\Coin
    // Mac: ~/Library/Application Support/Coin
    // Unix: ~/.Coin
#ifdef WIN32
    // Windows
    return GetSpecialFolderPath(CSIDL_APPDATA) / IniCfg().GetCoinName();
#else
    fs::path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pathRet = fs::path("/");
    else
        pathRet = fs::path(pszHome);
#ifdef MAC_OSX
    // Mac
    pathRet /= "Library/Application Support";
    TryCreateDirectory(pathRet);
    return pathRet / IniCfg().GetCoinName();
#else
    // Unix
    return pathRet / ("." + IniCfg().GetCoinName());
#endif
#endif
}

static boost::filesystem::path pathCached[MAX_NETWORK_TYPES + 1];
static CCriticalSection csPathCached;

const boost::filesystem::path& GetDataDir(bool fNetSpecific) {
    namespace fs = boost::filesystem;

    LOCK(csPathCached);

    int nNet = MAX_NETWORK_TYPES;
    if (fNetSpecific) nNet = SysCfg().NetworkID();

    fs::path& path = pathCached[nNet];

    // This can be called during exceptions by LogPrint("INFO",), so we cache the
    // value so we don't have to do memory allocations after that.
    if (!path.empty()) return path;

    if (CBaseParams::IsArgCount("-datadir")) {
        std::string strCfgDataDir = CBaseParams::GetArg("-datadir", "");
        if ("cur" == strCfgDataDir) {
            strCfgDataDir = fs::initial_path<boost::filesystem::path>().string();
        }
        path = fs::system_complete(strCfgDataDir);
        if (!fs::is_directory(path)) {
            path = "";
            return path;
        }
    } else {
        path = GetDefaultDataDir();
    }
    if (fNetSpecific) path /= SysCfg().DataDir();

    fs::create_directories(path);

    return path;
}

void ClearDatadirCache() {
    fill(&pathCached[0], &pathCached[MAX_NETWORK_TYPES + 1], boost::filesystem::path());
}

boost::filesystem::path GetConfigFile() {
    boost::filesystem::path pathConfigFile(
        CBaseParams::GetArg("-conf", IniCfg().GetCoinName() + ".conf"));
    if (!pathConfigFile.is_complete()) pathConfigFile = GetDataDir(false) / pathConfigFile;

    return pathConfigFile;
}

boost::filesystem::path GetAbsolutePath(const string& path) {
    boost::system::error_code ec;
    return canonical(boost::filesystem::path(path), ec);
}

void ReadConfigFile(map<string, string>& mapSettingsRet,
                    map<string, vector<string>>& mapMultiSettingsRet) {
    boost::filesystem::ifstream streamConfig(GetConfigFile());
    if (!streamConfig.good()) return;  // No WaykiChain.conf file is OK

    set<string> setOptions;
    setOptions.insert("*");

    for (boost::program_options::detail::config_file_iterator it(streamConfig, setOptions), end;
         it != end; ++it) {
        // Don't overwrite existing settings so command line settings override WaykiChain.conf
        string strKey = string("-") + it->string_key;
        if (mapSettingsRet.count(strKey) == 0) {
            mapSettingsRet[strKey] = it->value[0];

            // interpret nofoo=1 as foo=0 (and nofoo=0 as foo=1) as long as foo not set)
            InterpretNegativeSetting(strKey, mapSettingsRet);
        }
        mapMultiSettingsRet[strKey].push_back(it->value[0]);
    }
    // If datadir is changed in .conf file:
    ClearDatadirCache();
}

boost::filesystem::path GetPidFile() {
    boost::filesystem::path pathPidFile(SysCfg().GetArg("-pid", "coind.pid"));
    if (!pathPidFile.is_complete()) pathPidFile = GetDataDir() / pathPidFile;
    return pathPidFile;
}

#ifndef WIN32
void CreatePidFile(const boost::filesystem::path& path, pid_t pid) {
    FILE* file = fopen(path.string().c_str(), "w");
    if (file) {
        fprintf(file, "%d\n", pid);
        fclose(file);
    }
}
#endif

bool RenameOver(boost::filesystem::path src, boost::filesystem::path dest) {
#ifdef WIN32
    return MoveFileExA(src.string().c_str(), dest.string().c_str(), MOVEFILE_REPLACE_EXISTING);
#else
    int rc = rename(src.string().c_str(), dest.string().c_str());
    return (rc == 0);
#endif /* WIN32 */
}

// Ignores exceptions thrown by boost's create_directory if the requested directory exists.
//   Specifically handles case where path p exists, but it wasn't possible for the user to write to
//   the parent directory.
bool TryCreateDirectory(const boost::filesystem::path& p) {
    try {
        return boost::filesystem::create_directory(p);
    } catch (boost::filesystem::filesystem_error) {
        if (!boost::filesystem::exists(p) || !boost::filesystem::is_directory(p)) throw;
    }

    // create_directory didn't create the directory, it had to have existed already
    return false;
}

void FileCommit(FILE* fileout) {
    fflush(fileout);  // harmless if redundantly called
#ifdef WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(fileout));
    FlushFileBuffers(hFile);
#else
#if defined(__linux__) || defined(__NetBSD__)
    fdatasync(fileno(fileout));
#elif defined(__APPLE__) && defined(F_FULLFSYNC)
    fcntl(fileno(fileout), F_FULLFSYNC, 0);
#else
    fsync(fileno(fileout));
#endif
#endif
}

bool TruncateFile(FILE* file, unsigned int length) {
#if defined(WIN32)
    return _chsize(_fileno(file), length) == 0;
#else
    return ftruncate(fileno(file), length) == 0;
#endif
}

// this function tries to raise the file descriptor limit to the requested number.
// It returns the actual file descriptor limit (which may be more or less than nMinFD)
int RaiseFileDescriptorLimit(int nMinFD) {
#if defined(WIN32)
    return 2048;
#else
    struct rlimit limitFD;
    if (getrlimit(RLIMIT_NOFILE, &limitFD) != -1) {
        if (limitFD.rlim_cur < (rlim_t)nMinFD) {
            limitFD.rlim_cur = nMinFD;
            if (limitFD.rlim_cur > limitFD.rlim_max) limitFD.rlim_cur = limitFD.rlim_max;
            setrlimit(RLIMIT_NOFILE, &limitFD);
            getrlimit(RLIMIT_NOFILE, &limitFD);
        }
        return limitFD.rlim_cur;
    }
    return nMinFD;  // getrlimit failed, assume it's fine
#endif
}

// this function tries to make a particular range of a file allocated (corresponding to disk space)
// it is advisory, and the range specified in the arguments will never contain live data
void AllocateFileRange(FILE* file, unsigned int offset, unsigned int length) {
#if defined(WIN32)
    // Windows-specific version
    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
    LARGE_INTEGER nFileSize;
    int64_t nEndPos      = (int64_t)offset + length;
    nFileSize.u.LowPart  = nEndPos & 0xFFFFFFFF;
    nFileSize.u.HighPart = nEndPos >> 32;
    SetFilePointerEx(hFile, nFileSize, 0, FILE_BEGIN);
    SetEndOfFile(hFile);
#elif defined(MAC_OSX)
    // OSX specific version
    fstore_t fst;
    fst.fst_flags      = F_ALLOCATECONTIG;
    fst.fst_posmode    = F_PEOFPOSMODE;
    fst.fst_offset     = 0;
    fst.fst_length     = (off_t)offset + length;
    fst.fst_bytesalloc = 0;
    if (fcntl(fileno(file), F_PREALLOCATE, &fst) == -1) {
        fst.fst_flags = F_ALLOCATEALL;
        fcntl(fileno(file), F_PREALLOCATE, &fst);
    }
    ftruncate(fileno(file), fst.fst_length);
#elif defined(__linux__)
    // Version using posix_fallocate
    off_t nEndPos = (off_t)offset + length;
    posix_fallocate(fileno(file), 0, nEndPos);
#else
    // Fallback version
    // TODO: just write one byte per block
    static const char buf[65536] = {};
    fseek(file, offset, SEEK_SET);
    while (length > 0) {
        unsigned int now = 65536;
        if (length < now) now = length;
        fwrite(buf, 1, now, file);  // allowed to fail; this function is advisory anyway
        length -= now;
    }
#endif
}

void ShrinkDebugFile() {
    // Scroll debug.log if it's getting too big
    boost::filesystem::path pathLog = GetDataDir() / "debug.log";
    FILE* file                      = fopen(pathLog.string().c_str(), "r");
    if (file && boost::filesystem::file_size(pathLog) > 10 * 1000000) {
        // Restart the file with some of the end
        char pch[200000];
        fseek(file, -sizeof(pch), SEEK_END);
        int nBytes = fread(pch, 1, sizeof(pch), file);
        fclose(file);

        file = fopen(pathLog.string().c_str(), "w");
        if (file) {
            fwrite(pch, 1, nBytes, file);
            fclose(file);
        }
    } else if (file != NULL)
        fclose(file);
}

//
// "Never go to sea with two chronometers; take one or three."
// Our three time sources are:
//  - System clock
//  - Median of other nodes clocks
//  - The user (asking the user to fix the system clock if the first two disagree)
//
static int64_t nMockTime = 0;  // For unit testing

int64_t GetTime() {
    if (nMockTime) return nMockTime;

    return time(NULL);
}

void SetMockTime(int64_t nMockTimeIn) { nMockTime = nMockTimeIn; }

static CCriticalSection cs_nTimeOffset;
static int64_t nTimeOffset = 0;

int64_t GetTimeOffset() {
    LOCK(cs_nTimeOffset);
    return nTimeOffset;
}

int64_t GetAdjustedTime() { return GetTime() + GetTimeOffset(); }

void AddTimeData(const CNetAddr& ip, int64_t nTime) {
    int64_t nOffsetSample = nTime - GetTime();

    LOCK(cs_nTimeOffset);
    // Ignore duplicates
    static set<CNetAddr> setKnown;
    if (!setKnown.insert(ip).second) return;

    // Add data
    static CMedianFilter<int64_t> vTimeOffsets(200, 0);
    vTimeOffsets.input(nOffsetSample);
    LogPrint("INFO", "Added time data, samples %d, offset %+d (%+d minutes)\n", vTimeOffsets.size(),
             nOffsetSample, nOffsetSample / 60);

    if (vTimeOffsets.size() >= 5 && vTimeOffsets.size() % 2 == 1) {
        int64_t nMedian         = vTimeOffsets.median();
        vector<int64_t> vSorted = vTimeOffsets.sorted();
        // Only let other nodes change our time by so much
        if (abs64(nMedian) < 70) {
            nTimeOffset = nMedian;
        } else {
            nTimeOffset = 0;

            static bool fDone;
            if (!fDone) {
                // If nobody has a time different than ours but within 5 seconds of ours, give a
                // warning
                bool fMatch = false;
                for (int64_t nOffset : vSorted)
                    if (nOffset != 0 && abs64(nOffset) < 5) fMatch = true;

                if (!fMatch) {
                    fDone = true;
                    string strMessage =
                        _("Warning: Please check that your computer's date and time "
                          "are correct! If your clock is wrong Coin will not work properly.");
                    strMiscWarning = strMessage;
                    LogPrint("INFO", "*** %s\n", strMessage);
                }
            }
        }

        if (SysCfg().IsDebug()) {
            for (int64_t n : vSorted) LogPrint("DEBUG", "%+d  ", n);

            LogPrint("DEBUG", "|  ");
        }

        LogPrint("INFO", "nTimeOffset = %+d  (%+d minutes)\n", nTimeOffset, nTimeOffset / 60);
    }
}

string FormatVersion(int nVersion) {
    if (nVersion % 100 == 0)
        return strprintf("%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100,
                         (nVersion / 100) % 100);
    else
        return strprintf("%d.%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100,
                         (nVersion / 100) % 100, nVersion % 100);
}

string FormatFullVersion() { return CLIENT_BUILD; }

// Format the subversion field according to BIP 14 spec (https://en.bitcoin.it/wiki/BIP_0014)
string FormatSubVersion(const string& name, int nClientVersion, const vector<string>& comments) {
    ostringstream ss;
    ss << "/";
    ss << name << ":" << FormatVersion(nClientVersion);
    if (!comments.empty())
        ss << ":"
           << "(" << boost::algorithm::join(comments, "; ") << ")";
    ss << "/";
    return ss.str();
}

void StringReplace(string& strBase, string strSrc, string strDes) {
    string::size_type pos    = 0;
    string::size_type srcLen = strSrc.size();
    string::size_type desLen = strDes.size();
    pos                      = strBase.find(strSrc, pos);
    while ((pos != string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos = strBase.find(strSrc, (pos + desLen));
    }
}

#ifdef WIN32
boost::filesystem::path GetSpecialFolderPath(int nFolder, bool fCreate) {
    namespace fs = boost::filesystem;

    char pszPath[MAX_PATH] = "";

    if (SHGetSpecialFolderPathA(NULL, pszPath, nFolder, fCreate)) {
        return fs::path(pszPath);
    }

    LogPrint("INFO", "SHGetSpecialFolderPathA() failed, could not obtain requested path.\n");
    return fs::path("");
}
#endif

boost::filesystem::path GetTempPath() {
#if BOOST_FILESYSTEM_VERSION == 3
    return boost::filesystem::temp_directory_path();
#else
    // TODO: remove when we don't support filesystem v2 anymore
    boost::filesystem::path path;
#ifdef WIN32
    char pszPath[MAX_PATH] = "";

    if (GetTempPathA(MAX_PATH, pszPath)) path = boost::filesystem::path(pszPath);
#else
    path = boost::filesystem::path("/tmp");
#endif
    if (path.empty() || !boost::filesystem::is_directory(path)) {
        LogPrint("INFO", "GetTempPath(): failed to find temp path\n");
        return boost::filesystem::path("");
    }
    return path;
#endif
}

void runCommand(string strCommand) {
    int nErr = ::system(strCommand.c_str());
    if (nErr)
        LogPrint("INFO", "runCommand error: system(%s) returned %d\n", strCommand, nErr);
}

void RenameThread(const char* name) {
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif 0 && (defined(__FreeBSD__) || defined(__OpenBSD__))
    // TODO: This is currently disabled because it needs to be verified to work
    //       on FreeBSD or OpenBSD first. When verified the '0 &&' part can be
    //       removed.
    pthread_set_name_np(pthread_self(), name);

#elif defined(MAC_OSX) && defined(__MAC_OS_X_VERSION_MAX_ALLOWED)

// pthread_setname_np is XCode 10.6-and-later
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
    pthread_setname_np(name);
#endif

#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}

void SetupEnvironment() {
#ifndef WIN32
    try {
#if BOOST_FILESYSTEM_VERSION == 3
        boost::filesystem::path::codecvt();  // Raises runtime error if current locale is invalid
#else                                        // boost filesystem v2
        locale();  // Raises runtime error if current locale is invalid
#endif
    } catch (runtime_error& e) {
        setenv("LC_ALL", "C", 1);  // Force C locale
    }
#endif
}

string DateTimeStrFormat(const char* pszFormat, int64_t nTime) {
    // locale takes ownership of the pointer
    locale loc(locale::classic(), new boost::posix_time::time_facet(pszFormat));
    stringstream ss;
    ss.imbue(loc);
    ss << boost::posix_time::from_time_t(nTime);
    return ss.str();
}


static bool ParsePrechecks(const std::string& str)
{
    if (str.empty()) // No empty string allowed
        return false;
    if (str.size() >= 1 && (IsSpace(str[0]) || IsSpace(str[str.size()-1]))) // No padding allowed
        return false;
    if (str.size() != strlen(str.c_str())) // No embedded NUL characters allowed
        return false;
    return true;
}

bool ParseInt32(const std::string& str, int32_t *out)
{
    if (!ParsePrechecks(str))
        return false;
    char *endp = nullptr;
    errno = 0; // strtol will not set errno if valid
    long int n = strtol(str.c_str(), &endp, 10);
    if(out) *out = (int32_t)n;
    // Note that strtol returns a *long int*, so even if strtol doesn't report an over/underflow
    // we still have to check that the returned value is within the range of an *int32_t*. On 64-bit
    // platforms the size of these types may be different.
    return endp && *endp == 0 && !errno &&
        n >= std::numeric_limits<int32_t>::min() &&
        n <= std::numeric_limits<int32_t>::max();
}