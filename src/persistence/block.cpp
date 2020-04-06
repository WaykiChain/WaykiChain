// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "block.h"

#include "entities/account.h"
#include "tx/blockpricemediantx.h"
#include "main.h"
#include "net.h"

uint256 CBlockHeader::GetHash() const {
    return ComputeSignatureHash();
}

void CBlockHeader::SetHeight(uint32_t  height) {
    this->height = height;
}

uint256 CBlockHeader::ComputeSignatureHash() const {
    CHashWriter ss(SER_GETHASH, CLIENT_VERSION);
    ss << nVersion << prevBlockHash << merkleRootHash << nTime << nNonce << height << nFuel << nFuelRate;
    return ss.GetHash();
}

uint256 CBlock::BuildMerkleTree() const {
    vMerkleTree.clear();
    for (const auto& ptx : vptx) {
        vMerkleTree.push_back(ptx->GetHash());
    }
    int32_t j = 0;
    for (int32_t nSize = vptx.size(); nSize > 1; nSize = (nSize + 1) / 2) {
        for (int32_t i = 0; i < nSize; i += 2) {
            int32_t i2 = min(i + 1, nSize - 1);
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j + i]), END(vMerkleTree[j + i]),
                                       BEGIN(vMerkleTree[j + i2]), END(vMerkleTree[j + i2])));
        }
        j += nSize;
    }
    return (vMerkleTree.empty() ? uint256() : vMerkleTree.back());
}

vector<uint256> CBlock::GetMerkleBranch(int32_t index) const {
    if (vMerkleTree.empty())
        BuildMerkleTree();
    vector<uint256> vMerkleBranch;
    int32_t j = 0;
    for (int32_t nSize = vptx.size(); nSize > 1; nSize = (nSize + 1) / 2) {
        int32_t i = min(index ^ 1, nSize - 1);
        vMerkleBranch.push_back(vMerkleTree[j + i]);
        index >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

uint256 CBlock::CheckMerkleBranch(uint256 hash, const vector<uint256>& vMerkleBranch, int32_t index) {
    if (index == -1)
        return uint256();
    for (const auto& otherside : vMerkleBranch) {
        if (index & 1)
            hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
        index >>= 1;
    }
    return hash;
}

map<TokenSymbol, uint64_t> CBlock::GetFees() const {
    map<TokenSymbol, uint64_t> fees = {{SYMB::WICC, 0}, {SYMB::WUSD, 0}};
    for (uint32_t i = 1; i < vptx.size(); ++i) {
        auto fees_symbol = std::get<0>(vptx[i]->GetFees());
        assert(fees_symbol == SYMB::WICC || fees_symbol == SYMB::WUSD);
        fees[fees_symbol] = std::get<1>(vptx[i]->GetFees());
    }

    return fees;
}

const PriceMap& CBlock::GetBlockMedianPrice() const {
    if (GetFeatureForkVersion(GetHeight()) == MAJOR_VER_R1) {
        return EMPTY_PRICE_MAP;
    }

    for (size_t index = 1; index < vptx.size(); ++ index) {
        if (vptx[index]->IsPriceFeedTx())
            continue;

        if (vptx[index]->IsPriceMedianTx())
            return ((CBlockPriceMedianTx*) vptx[index].get())->median_prices;

        break;
    }

    LogPrint(BCLog::ERROR, "GetBlockMedianPrice() : failed to acquire median price, height: %u, hash: %s\n", GetHeight(),
             GetHash().GetHex());
    assert(false && "block has NO price median tx");
    return EMPTY_PRICE_MAP;
}

CUserID CBlock::GetMinerUserID() const {
    assert(vptx.size() > 0);
    assert(vptx[0]->IsBlockRewardTx());

    return vptx[0]->txUid;
}

void CBlock::Print() const {
    string medianPrices;
    const auto& priceMap = GetBlockMedianPrice();
    for (const auto &item : priceMap) {
        medianPrices += strprintf("{%s/%s -> %llu}", std::get<0>(item.first), std::get<1>(item.first), item.second);
    }

    LogPrint(BCLog::DEBUG, "[%d] hash=%.7s**, ver=%d, hashPrevBlock=%s, merkleRootHash=%s, nTime=%u, nNonce=%u, vtx=%u, nFuel=%d, "
             "nFuelRate=%d, median prices: %s\n",
             height, GetHash().ToString(), nVersion, prevBlockHash.ToString(), merkleRootHash.ToString(), nTime, nNonce,
             vptx.size(), nFuel, nFuelRate, medianPrices);
}

std::tuple<bool, int> CBlock::GetTxIndex(const uint256& txid) const {
    for (size_t i = 0; i < vMerkleTree.size(); i++) {
        if (txid == vMerkleTree[i]) {
            return std::make_tuple(true, i);
        }
    }

    return std::make_tuple(false, 0);
}

//////////////////////////////////////////////////////////////////////////////
// global functions

bool WriteBlockToDisk(CBlock &block, CDiskBlockPos &pos) {
    // Open history file to append
    CAutoFile fileout = CAutoFile(OpenBlockFile(pos), SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return ERRORMSG("WriteBlockToDisk : OpenBlockFile failed");

    // Write index header
    uint32_t nSize = fileout.GetSerializeSize(block);
    fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

    // Write block
    int32_t fileOutPos = ftell(fileout);
    if (fileOutPos < 0)
        return ERRORMSG("WriteBlockToDisk : ftell failed");
    pos.nPos = (uint32_t)fileOutPos;
    fileout << block;

    // Flush stdio buffers and commit to disk before returning
    fflush(fileout);
    if (!IsInitialBlockDownload())
        FileCommit(fileout);

    return true;
}

bool ReadBlockFromDisk(const CDiskBlockPos &pos, CBlock &block) {
    block.SetNull();

    // Open history file to read
    CAutoFile filein = CAutoFile(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (!filein)
        return ERRORMSG("ReadBlockFromDisk : OpenBlockFile failed");

    // Read block
    try {
        filein >> block;
    } catch (std::exception &e) {
        return ERRORMSG("Deserialize or I/O error - %s", e.what());
    }

    return true;
}

bool ReadBlockFromDisk(const CBlockIndex *pIndex, CBlock &block) {
    if (!ReadBlockFromDisk(pIndex->GetBlockPos(), block))
        return false;

    if (block.GetHash() != pIndex->GetBlockHash())
        return ERRORMSG("ReadBlockFromDisk(CBlock&, CBlockIndex*) : GetHash() doesn't match");

    return true;
}

bool ReadBaseTxFromDisk(const CTxCord txCord, std::shared_ptr<CBaseTx> &pTx) {
    auto pBlock = std::make_shared<CBlock>();
    const CBlockIndex* pBlockIndex = chainActive[ txCord.GetHeight() ];
    if (pBlockIndex == nullptr) {
        return ERRORMSG("ReadBaseTxFromDisk error, the height(%d) is exceed current best block height", txCord.GetHeight());
    }
    if (!ReadBlockFromDisk(pBlockIndex, *pBlock)) {
        return ERRORMSG("ReadBaseTxFromDisk error, read the block at height(%d) failed!", txCord.GetHeight());
    }
    if (txCord.GetIndex() >= pBlock->vptx.size()) {
        return ERRORMSG("ReadBaseTxFromDisk error, the tx(%s) index exceed the tx count of block", txCord.ToString());
    }
    pTx = pBlock->vptx.at(txCord.GetIndex())->GetNewInstance();
    return true;
}
