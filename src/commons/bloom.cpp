// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bloom.h"

#include "crypto/hash.h"
#include "main.h"

#define LN2SQUARED 0.4804530139182014246671025263266649717305529515945455
#define LN2 0.6931471805599453094172321214581765680755001343602552

CBloomFilter::CBloomFilter(uint32_t nElements, double nFPRate, uint32_t nTweakIn, uint8_t nFlagsIn)
    :  // The ideal size for a bloom filter with a given number of elements and false positive rate is:
       // - nElements * log(fp rate) / ln(2)^2
       // We ignore filter parameters which will create a bloom filter larger than the protocol limits
      vData(min((uint32_t)(-1 / LN2SQUARED * nElements * log(nFPRate)), MAX_BLOOM_FILTER_SIZE * 8) / 8),
      // The ideal number of hash functions is filter size * ln(2) / number of elements
      // Again, we ignore filter parameters which will create a bloom filter with more hash functions than the protocol
      // limits See http://en.wikipedia.org/wiki/Bloom_filter for an explanation of these formulas
      isFull(false),
      isEmpty(false),
      nHashFuncs(min((uint32_t)(vData.size() * 8 / nElements * LN2), MAX_HASH_FUNCS)),
      nTweak(nTweakIn),
      nFlags(nFlagsIn) {}

inline uint32_t CBloomFilter::Hash(uint32_t nHashNum, const vector<uint8_t>& vDataToHash) const {
    // 0xFBA4C795 chosen as it guarantees a reasonable bit difference between nHashNum values.
    return MurmurHash3(nHashNum * 0xFBA4C795 + nTweak, vDataToHash) % (vData.size() * 8);
}

void CBloomFilter::insert(const vector<uint8_t>& vKey) {
    if (isFull)
        return;
    for (uint32_t i = 0; i < nHashFuncs; i++) {
        uint32_t index = Hash(i, vKey);
        // Sets bit index of vData
        vData[index >> 3] |= (1 << (7 & index));
    }
    isEmpty = false;
}

void CBloomFilter::insert(const uint256& hash) {
    vector<uint8_t> data(hash.begin(), hash.end());
    insert(data);
}

bool CBloomFilter::contains(const vector<uint8_t>& vKey) const {
    if (isFull)
        return true;
    if (isEmpty)
        return false;
    for (uint32_t i = 0; i < nHashFuncs; i++) {
        uint32_t index = Hash(i, vKey);
        // Checks bit index of vData
        if (!(vData[index >> 3] & (1 << (7 & index))))
            return false;
    }
    return true;
}

bool CBloomFilter::contains(const uint256& hash) const {
    vector<uint8_t> data(hash.begin(), hash.end());
    return contains(data);
}

bool CBloomFilter::IsWithinSizeConstraints() const {
    return vData.size() <= MAX_BLOOM_FILTER_SIZE && nHashFuncs <= MAX_HASH_FUNCS;
}

bool CBloomFilter::IsRelevantAndUpdate(CBaseTx* pBaseTx, const uint256& hash) {
    //    bool fFound = false;
    // Match if the filter contains the hash of tx
    //  for finding tx when they appear in a block
    if (isFull)
        return true;
    if (isEmpty)
        return false;
    //    if (contains(hash))
    //        fFound = true;

    //    for (uint32_t i = 0; i < tx.vout.size(); i++)
    //    {
    //        const CTxOut& txout = tx.vout[i];
    //        // Match if the filter contains any arbitrary script data element in any scriptPubKey in tx
    //        // If this matches, also add the specific output that was matched.
    //        // This means clients don't have to update the filter themselves when a new relevant tx
    //        // is discovered in order to find spending transactions, which avoids round-tripping and race conditions.
    //        CScript::const_iterator pc = txout.scriptPubKey.begin();
    //        vector<uint8_t> data;
    //        while (pc < txout.scriptPubKey.end())
    //        {
    //            opcodetype opcode;
    //            if (!txout.scriptPubKey.GetOp(pc, opcode, data))
    //                break;
    //            if (data.size() != 0 && contains(data))
    //            {
    //                fFound = true;
    //                if ((nFlags & BLOOM_UPDATE_MASK) == BLOOM_UPDATE_ALL)
    //                    insert(COutPoint(hash, i));
    //                else if ((nFlags & BLOOM_UPDATE_MASK) == BLOOM_UPDATE_P2PUBKEY_ONLY)
    //                {
    //                    txnouttype type;
    //                    vector<vector<uint8_t> > vSolutions;
    //                    if (Solver(txout.scriptPubKey, type, vSolutions) &&
    //                            (type == TX_PUBKEY || type == TX_MULTISIG))
    //                        insert(COutPoint(hash, i));
    //                }
    //                break;
    //            }
    //        }
    //    }
    //
    //    if (fFound)
    //        return true;
    //
    //    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    //    {
    //        // Match if the filter contains an outpoint tx spends
    //        if (contains(txin.prevout))
    //            return true;
    //
    //        // Match if the filter contains any arbitrary script data element in any scriptSig in tx
    //        CScript::const_iterator pc = txin.scriptSig.begin();
    //        vector<uint8_t> data;
    //        while (pc < txin.scriptSig.end())
    //        {
    //            opcodetype opcode;
    //            if (!txin.scriptSig.GetOp(pc, opcode, data))
    //                break;
    //            if (data.size() != 0 && contains(data))
    //                return true;
    //        }
    //    }

    return false;
}

void CBloomFilter::UpdateEmptyFull() {
    bool full  = true;
    bool empty = true;
    for (uint32_t i = 0; i < vData.size(); i++) {
        full &= vData[i] == 0xff;
        empty &= vData[i] == 0;
    }
    isFull  = full;
    isEmpty = empty;
}

void CBloomFilter::Clear() {
    vData.assign(vData.size(), 0);
    isFull  = false;
    isEmpty = true;
}
