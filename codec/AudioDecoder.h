#pragma once
#include "memory/Packet.h"
#include <cstdint>

namespace codec
{

// audio decoder interface.
// RTP payload => 48kHz stereo
// Packet concealment can cause output to contain multiple audio frames
class AudioDecoder
{
public:
    // return # decoded pcm16 stereo frames
    // May produce PLC data if seqno has gap
    virtual int32_t decodePacket(uint32_t extendedSequenceNumber,
        const unsigned char* payload,
        size_t payloadLength,
        int16_t* audioData,
        size_t audioBufferFrames) = 0;

    virtual void onUnusedPacketReceived(uint32_t extendedSequenceNumber) = 0;

    // produce PLC from previous packets or silence
    virtual int32_t conceal(int16_t* audioData, size_t audioBufferFrames) = 0;
};

} // namespace codec