#pragma once

#include <cstdint>
#include <stddef.h>

#include "codec/AudioDecoder.h"

namespace codec
{

class OpusDecoder final : public AudioDecoder
{
public:
    OpusDecoder();
    ~OpusDecoder();

    bool isInitialized() const { return _initialized; }

    int32_t decodePacket(uint32_t extendedSequenceNumber,
        const unsigned char* payload,
        size_t payloadLength,
        int16_t* audioData,
        size_t pcmSampleCount) override;

    void onUnusedPacketReceived(uint32_t extendedSequenceNumber) override;
    int32_t conceal(int16_t* audioData, size_t audioBufferFrames) override;

private:
    bool hasDecoded() const { return _hasDecodedPacket; }

    int32_t conceal(const unsigned char* payloadStart,
        int32_t payloadLength,
        int16_t* decodedData,
        size_t pcmSampleCount);

    uint32_t getExpectedSequenceNumber() const { return _sequenceNumber + 1; }

    int32_t getLastPacketDuration();
    int32_t decode(uint32_t extendedSequenceNumber,
        const unsigned char* payloadStart,
        int32_t payloadLength,
        unsigned char* decodedData,
        const size_t framesInDecodedPacket);

    struct OpaqueDecoderState;

    bool _initialized;
    OpaqueDecoderState* _state;
    uint32_t _sequenceNumber;
    bool _hasDecodedPacket;
};

} // namespace codec
