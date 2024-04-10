#pragma once
#include "memory/Packet.h"
#include <cstdint>

namespace codec
{
class AudioDecoder
{
public:
    virtual int32_t decodePacket(uint32_t extendedSequenceNumber,
        uint64_t timestamp,
        const unsigned char* payload,
        size_t payloadLength,
        int16_t* audioData,
        size_t audioBufferFrames) = 0;

    virtual void onUnusedPacketReceived(uint32_t extendedSequenceNumber) = 0;
};

} // namespace codec