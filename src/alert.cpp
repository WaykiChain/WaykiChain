// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "alert.h"

#include "accounts/key.h"
#include "net.h"
#include "util.h"

#include <stdint.h>
#include <algorithm>
#include <map>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace std;

map<uint256, CAlert> mapAlerts;
CCriticalSection cs_mapAlerts;

void CUnsignedAlert::SetNull()
{
    nVersion = 1;
    nRelayUntil = 0;
    nExpiration = 0;
    nID = 0;
    nCancel = 0;
    setCancel.clear();
    nMinVer = 0;
    nMaxVer = 0;
    setSubVer.clear();
    nPriority = 0;

    strComment.clear();
    strStatusBar.clear();
    strReserved.clear();
}

string CUnsignedAlert::ToString() const
{
    string strSetCancel;
    for(auto n:setCancel)
        strSetCancel += strprintf("%d ", n);

    string strSetSubVer;
    for(auto str:setSubVer)
        strSetSubVer += "\"" + str + "\" ";

    return strprintf(
        "CAlert(\n"
        "    nVersion     = %d\n"
        "    nRelayUntil  = %d\n"
        "    nExpiration  = %d\n"
        "    nID          = %d\n"
        "    nCancel      = %d\n"
        "    setCancel    = %s\n"
        "    nMinVer      = %d\n"
        "    nMaxVer      = %d\n"
        "    setSubVer    = %s\n"
        "    nPriority    = %d\n"
        "    strComment   = \"%s\"\n"
        "    strStatusBar = \"%s\"\n"
        ")\n",
        nVersion,
        nRelayUntil,
        nExpiration,
        nID,
        nCancel,
        strSetCancel,
        nMinVer,
        nMaxVer,
        strSetSubVer,
        nPriority,
        strComment,
        strStatusBar);
}

void CUnsignedAlert::Print() const
{
    LogPrint("INFO","%s", ToString());
}

void CAlert::SetNull()
{
    CUnsignedAlert::SetNull();
    vchMsg.clear();
    vchSig.clear();
}

bool CAlert::IsNull() const
{
    return (nExpiration == 0);
}

uint256 CAlert::GetHash() const
{
    return Hash(this->vchMsg.begin(), this->vchMsg.end());
}

bool CAlert::IsInEffect() const
{
    return (GetAdjustedTime() < nExpiration);
}

bool CAlert::Cancels(const CAlert& alert) const
{
    if (!IsInEffect())
        return false; // this was a no-op before 31403
    return (alert.nID <= nCancel || setCancel.count(alert.nID));
}

bool CAlert::AppliesTo(int nVersion, string strSubVerIn) const
{
    // TODO: rework for client-version-embedded-in-strSubVer ?
    return (IsInEffect() &&
            nMinVer <= nVersion && nVersion <= nMaxVer &&
            (setSubVer.empty() || setSubVer.count(strSubVerIn)));
}

bool CAlert::AppliesToMe() const
{
    return AppliesTo(PROTOCOL_VERSION, FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, vector<string>()));
}

bool CAlert::RelayTo(CNode* pNode) const
{
    if (!IsInEffect())
        return false;
    // returns true if wasn't already contained in the set
    if (pNode->setKnown.insert(GetHash()).second) {
        if (AppliesTo(pNode->nVersion, pNode->strSubVer) ||
            AppliesToMe() ||
            GetAdjustedTime() < nRelayUntil) {
            pNode->PushMessage("alert", *this);
            return true;
        }
    }
    return false;
}

bool CAlert::CheckSignature() const
{
    CPubKey key(SysCfg().AlertKey());
    if (!key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig))
        return ERRORMSG("CAlert::CheckSignature() : verify signature failed");

    // Now unserialize the data
    CDataStream sMsg(vchMsg, SER_NETWORK, PROTOCOL_VERSION);
    sMsg >> *(CUnsignedAlert*)this;
    return true;
}

CAlert CAlert::getAlertByHash(const uint256 &hash)
{
    CAlert retval;
    {
        LOCK(cs_mapAlerts);
        map<uint256, CAlert>::iterator mi = mapAlerts.find(hash);
        if(mi != mapAlerts.end())
            retval = mi->second;
    }
    return retval;
}

bool CAlert::ProcessAlert(bool fThread)
{
    if (!CheckSignature())
        return false;
    if (!IsInEffect())
        return false;

    // alert.nID=max is reserved for if the alert key is
    // compromised. It must have a pre-defined message,
    // must never expire, must apply to all versions,
    // and must cancel all previous
    // alerts or it will be ignored (so an attacker can't
    // send an "everything is OK, don't panic" version that
    // cannot be overridden):
    int maxInt = numeric_limits<int>::max();
    if (nID == maxInt)
    {
        if (!(
                nExpiration == maxInt &&
                nCancel == (maxInt-1) &&
                nMinVer == 0 &&
                nMaxVer == maxInt &&
                setSubVer.empty() &&
                nPriority == maxInt &&
                strStatusBar == "URGENT: Alert key compromised, upgrade required"
                ))
            return false;
    }

    {
        LOCK(cs_mapAlerts);
        // Cancel previous alerts
        for (map<uint256, CAlert>::iterator mi = mapAlerts.begin(); mi != mapAlerts.end();) {
            const CAlert& alert = (*mi).second;
            if (Cancels(alert)) {
                LogPrint("alert", "cancelling alert %d\n", alert.nID);
                mapAlerts.erase(mi++);
            } else if (!alert.IsInEffect()) {
                LogPrint("alert", "expiring alert %d\n", alert.nID);
                mapAlerts.erase(mi++);
            } else {
                mi++;
            }
        }

        // Check if this alert has been cancelled
        for (const auto &item : mapAlerts) {
            const CAlert& alert = item.second;
            if (alert.Cancels(*this)) {
                LogPrint("alert", "alert already cancelled by %d\n", alert.nID);
                return false;
            }
        }

        // Add to mapAlerts
        mapAlerts.insert(make_pair(GetHash(), *this));
        // Notify UI and -alertnotify if it applies to me
        if (AppliesToMe()) {
            string strCmd = SysCfg().GetArg("-alertnotify", "");
            if (!strCmd.empty()) {
                // Alert text should be plain ascii coming from a trusted source, but to
                // be safe we first strip anything not in safeChars, then add single quotes around
                // the whole string before passing it to the shell:
                string singleQuote("'");
                string safeStatus = SanitizeString(strStatusBar);
                safeStatus        = singleQuote + safeStatus + singleQuote;
                boost::replace_all(strCmd, "%s", safeStatus);

                if (fThread)
                    boost::thread t(runCommand, strCmd);  // thread runs free
                else
                    runCommand(strCmd);
            }
        }
    }

    LogPrint("alert", "accepted alert %d, AppliesToMe()=%d\n", nID, AppliesToMe());
    return true;
}
