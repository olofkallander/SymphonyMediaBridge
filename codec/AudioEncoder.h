#pragma once
#include "memory/Packet.h"
#include <cstdint>

namespace codec
{

// audio encoder interface.
// 48kHz stereo => RTP payload
class AudioEncoder
{
public:
    // return length of encoded payload in bytes
    virtual int32_t encode(const int16_t* pcm16Stereo,
        const size_t frames,
        unsigned char* payloadStart,
        const size_t payloadMaxFrames) = 0;
};

} // namespace codec