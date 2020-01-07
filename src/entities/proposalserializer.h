// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_PROPOSALSERIALIZER_H
#define ENTITIES_PROPOSALSERIALIZER_H

#include "commons/serialize.h"
#include "entities/proposal.h"
using namespace std;

template<typename Stream> void CProposal::SerializePtr(Stream& os, const shared_ptr<CProposal>& pa, int nType, int nVersion){
    uint8_t proptype = pa->proposal_type;

    Serialize(os, proptype, nType, nVersion);
    switch (pa->proposal_type) {
        case NULL_PROPOSAL:
            Serialize(os,*((CNullProposal *)(pa.get())), nType, nVersion);
            break;
        case PARAM_GOVERN:
            Serialize(os, *((CParamsGovernProposal *) (pa.get())), nType, nVersion);
            break;
        case GOVERNER_UPDATE:
            Serialize(os, *((CGovernerUpdateProposal *) (pa.get())), nType, nVersion);
            break;
        default:
            throw ios_base::failure(strprintf("Serialize: proposalType(%d) error.",
                                              pa->proposal_type));
    }

}
template<typename Stream> void CProposal::UnserializePtr(Stream& is, std::shared_ptr<CProposal> &pa, int nType, int nVersion){
    uint8_t nProposalTye;
    is.read((char *)&(nProposalTye), sizeof(nProposalTye));
    switch((ProposalType)nProposalTye) {
        case NULL_PROPOSAL: {
            pa = std::make_shared<CNullProposal>() ;
            Unserialize(is, *((CNullProposal *)(pa.get())), nType, nVersion) ;
            break;

        }
        case PARAM_GOVERN: {
            pa = std::make_shared<CParamsGovernProposal>();
            Unserialize(is, *((CParamsGovernProposal *)(pa.get())), nType, nVersion);
            break;
        }

        case GOVERNER_UPDATE: {
            pa = std::make_shared<CGovernerUpdateProposal>();
            Unserialize(is, *((CGovernerUpdateProposal *)(pa.get())), nType, nVersion);
            break;
        }
        default:
            throw ios_base::failure(strprintf("Unserialize: nTxType(%d) error.",
                                              nProposalTye));
    }
    pa->proposal_type = ProposalType(nProposalTye);

}




#endif //ENTITIES_PROPOSALSERIALIZER_H
