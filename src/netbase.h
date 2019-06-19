// Copyright (c) 2009-2013 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_NETBASE_H
#define COIN_NETBASE_H

#if defined(HAVE_CONFIG_H)
#include "coin-config.h"
#endif

#include "commons/serialize.h"
#include "compat/compat.h"

#include <stdint.h>
#include <string>
#include <vector>

#ifdef WIN32
// In MSVC, this is defined as a macro, undefine it to prevent a compile and link error
#undef SetPort
#endif

enum Network {
    NET_UNROUTABLE,
    NET_IPV4,
    NET_IPV6,
    NET_TOR,

    NET_MAX,
};

extern bool fNameLookup;

/** IP address (IPv6, or IPv4 using mapped IPv6 range (::FFFF:0:0/96)) */
class CNetAddr {
protected:
    unsigned char ip[16];  // in network byte order

public:
    CNetAddr();
    CNetAddr(const struct in_addr& ipv4Addr);
    explicit CNetAddr(const char* pszIp, bool fAllowLookup = false);
    explicit CNetAddr(const std::string& strIp, bool fAllowLookup = false);
    void Init();
    void SetIP(const CNetAddr& ip);
    bool SetSpecial(const std::string& strName);  // for Tor addresses
    bool IsIPv4() const;                          // IPv4 mapped address (::FFFF:0:0/96, 0.0.0.0/0)
    bool IsIPv6() const;                          // IPv6 address (not mapped IPv4, not Tor)
    bool IsRFC1918() const;                       // IPv4 private networks (10.0.0.0/8, 192.168.0.0/16, 172.16.0.0/12)
    bool IsRFC3849() const;                       // IPv6 documentation address (2001:0DB8::/32)
    bool IsRFC3927() const;                       // IPv4 autoconfig (169.254.0.0/16)
    bool IsRFC3964() const;                       // IPv6 6to4 tunnelling (2002::/16)
    bool IsRFC4193() const;                       // IPv6 unique local (FC00::/7)
    bool IsRFC4380() const;                       // IPv6 Teredo tunnelling (2001::/32)
    bool IsRFC4843() const;                       // IPv6 ORCHID (2001:10::/28)
    bool IsRFC4862() const;                       // IPv6 autoconfig (FE80::/64)
    bool IsRFC6052() const;                       // IPv6 well-known prefix (64:FF9B::/96)
    bool IsRFC6145() const;                       // IPv6 IPv4-translated address (::FFFF:0:0:0/96)
    bool IsTor() const;
    bool IsLocal() const;
    bool IsRoutable() const;
    bool IsValid() const;
    bool IsMulticast() const;
    enum Network GetNetwork() const;
    std::string ToString() const;
    std::string ToStringIP() const;
    unsigned int GetByte(int n) const;
    uint64_t GetHash() const;
    bool GetInAddr(struct in_addr* pipv4Addr) const;
    std::vector<unsigned char> GetGroup() const;
    int GetReachabilityFrom(const CNetAddr* paddrPartner = nullptr) const;
    void Print() const;

    CNetAddr(const struct in6_addr& pipv6Addr);
    bool GetIn6Addr(struct in6_addr* pipv6Addr) const;

    friend bool operator==(const CNetAddr& a, const CNetAddr& b);
    friend bool operator!=(const CNetAddr& a, const CNetAddr& b);
    friend bool operator<(const CNetAddr& a, const CNetAddr& b);

    IMPLEMENT_SERIALIZE(READWRITE(FLATDATA(ip));)

    friend class CSubNet;
};

class CSubNet {
protected:
    /// Network (base) address
    CNetAddr network;
    /// Netmask, in network byte order
    uint8_t netmask[16];
    /// Is this value valid? (only used to signal parse errors)
    bool valid;

public:
    CSubNet();
    CSubNet(const CNetAddr& addr, int32_t mask);
    CSubNet(const CNetAddr& addr, const CNetAddr& mask);

    // constructor for single ip subnet (<ipv4>/32 or <ipv6>/128)
    explicit CSubNet(const CNetAddr& addr);

    bool Match(const CNetAddr& addr) const;

    std::string ToString() const;
    bool IsValid() const;

    friend bool operator==(const CSubNet& a, const CSubNet& b);
    friend bool operator!=(const CSubNet& a, const CSubNet& b) { return !(a == b); }
    friend bool operator<(const CSubNet& a, const CSubNet& b);

    IMPLEMENT_SERIALIZE(READWRITE(network); READWRITE(FLATDATA(netmask)); READWRITE(valid);)
};

/** A combination of a network address (CNetAddr) and a (TCP) port */
class CService : public CNetAddr {
protected:
    unsigned short port;  // host order

public:
    CService();
    CService(const CNetAddr& ip, unsigned short port);
    CService(const struct in_addr& ipv4Addr, unsigned short port);
    CService(const struct sockaddr_in& addr);
    explicit CService(const char* pszIpPort, int portDefault, bool fAllowLookup = false);
    explicit CService(const char* pszIpPort, bool fAllowLookup = false);
    explicit CService(const std::string& strIpPort, int portDefault, bool fAllowLookup = false);
    explicit CService(const std::string& strIpPort, bool fAllowLookup = false);
    void Init();
    void SetPort(unsigned short portIn);
    unsigned short GetPort() const;
    bool GetSockAddr(struct sockaddr* paddr, socklen_t* addrlen) const;
    bool SetSockAddr(const struct sockaddr* paddr);
    friend bool operator==(const CService& a, const CService& b);
    friend bool operator!=(const CService& a, const CService& b);
    friend bool operator<(const CService& a, const CService& b);
    std::vector<unsigned char> GetKey() const;
    std::string ToString() const;
    std::string ToStringPort() const;
    std::string ToStringIPPort() const;
    void Print() const;

    CService(const struct in6_addr& ipv6Addr, unsigned short port);
    CService(const struct sockaddr_in6& addr);

    IMPLEMENT_SERIALIZE(
        CService* pService = const_cast<CService*>(this);
        READWRITE(FLATDATA(ip));
        unsigned short portNumber = htons(port);
        READWRITE(portNumber);
        if (fRead)
            pService->port = ntohs(portNumber);
    )
};

typedef std::pair<CService, int> proxyType;
extern int GetConnectTime();
enum Network ParseNetwork(std::string net);
void SplitHostPort(std::string in, int& portOut, std::string& hostOut);
bool SetProxy(enum Network net, CService addrProxy, int nSocksVersion = 5);
bool GetProxy(enum Network net, proxyType& proxyInfoOut);
bool IsProxy(const CNetAddr& addr);
bool SetNameProxy(CService addrProxy, int nSocksVersion = 5);
bool HaveNameProxy();
bool LookupHost(const char* pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions = 0,
                bool fAllowLookup = true);
bool LookupHost(const char* pszName, CNetAddr& addr, bool fAllowLookup);
bool LookupHostNumeric(const char* pszName, std::vector<CNetAddr>& vIP, unsigned int nMaxSolutions = 0);
bool Lookup(const char* pszName, CService& addr, int portDefault = 0, bool fAllowLookup = true);
bool Lookup(const char* pszName, std::vector<CService>& vAddr, int portDefault = 0, bool fAllowLookup = true,
            unsigned int nMaxSolutions = 0);
bool LookupNumeric(const char* pszName, CService& addr, int portDefault = 0);
bool LookupSubNet(const char* pszName, CSubNet& subnet);
bool ConnectSocket(const CService& addr, SOCKET& hSocketRet, int nTimeout = GetConnectTime());
bool ConnectSocketByName(CService& addr, SOCKET& hSocketRet, const char* pszDest, int portDefault = 0,
                         int nTimeout = GetConnectTime());
/** Return readable error string for a network error code */
std::string NetworkErrorString(int err);

#endif
