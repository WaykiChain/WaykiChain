//
// Created by yehuan on 2019-12-11.
//

#include "netmessage.h"

int32_t CNetMessage::readHeader(const char* pch, uint32_t nBytes) {
    // copy data to temporary parsing buffer
    uint32_t nRemaining = 24 - nHdrPos;
    uint32_t nCopy      = min(nRemaining, nBytes);

    memcpy(&hdrbuf[nHdrPos], pch, nCopy);
    nHdrPos += nCopy;

    // if header incomplete, exit
    if (nHdrPos < 24)
        return nCopy;

    // deserialize to CMessageHeader
    try {
        hdrbuf >> hdr;
    } catch (std::exception& e) {
        return -1;
    }

    // reject messages larger than MAX_SIZE
    if (hdr.nMessageSize > MAX_SIZE)
        return -1;

    // switch state to reading message data
    in_data = true;
    vRecv.resize(hdr.nMessageSize);

    return nCopy;
}

int32_t CNetMessage::readData(const char* pch, uint32_t nBytes) {
    uint32_t nRemaining = hdr.nMessageSize - nDataPos;
    uint32_t nCopy      = min(nRemaining, nBytes);

    memcpy(&vRecv[nDataPos], pch, nCopy);
    nDataPos += nCopy;

    return nCopy;
}
