#include "codec/OpusDecoder.h"
#include "codec/Opus.h"
#include "utils/CheckedCast.h"
#include <algorithm>
#include <cassert>
#include <opus/opus.h>
#include <stddef.h>

namespace codec
{

struct OpusDecoder::OpaqueDecoderState
{
    ::OpusDecoder* _state;
};

OpusDecoder::OpusDecoder()
    : _initialized(false),
      _state(new OpaqueDecoderState{nullptr}),
      _sequenceNumber(0),
      _hasDecodedPacket(false)
{
    int32_t opusError = 0;
    _state->_state = opus_decoder_create(Opus::sampleRate, Opus::channelsPerFrame, &opusError);
    if (opusError != OPUS_OK)
    {
        return;
    }

    _initialized = true;
}

OpusDecoder::~OpusDecoder()
{
    if (_state->_state)
    {
        opus_decoder_destroy(_state->_state);
    }
    delete _state;
}

/** @return number of samples decoded
 * */
int32_t OpusDecoder::decode(uint32_t extendedSequenceNumber,
    const unsigned char* payloadStart,
    int32_t payloadLength,
    unsigned char* decodedData,
    const size_t pcmSampleCount)
{
    if (!_initialized)
    {
        assert(false);
        return -1;
    }
    assert(_state->_state);

    _sequenceNumber = extendedSequenceNumber;
    _hasDecodedPacket = true;

    return opus_decode(_state->_state,
        payloadStart,
        payloadLength,
        reinterpret_cast<int16_t*>(decodedData),
        utils::checkedCast<int32_t>(pcmSampleCount),
        0);
}

// re-construct packet before the most previously lost
int32_t OpusDecoder::conceal(int16_t* pcmData, const size_t pcmSampleCount)
{
    if (!_initialized)
    {
        assert(false);
        return -1;
    }
    assert(_state->_state);

    return opus_decode(_state->_state,
        nullptr,
        0,
        reinterpret_cast<int16_t*>(pcmData),
        std::min<int32_t>(pcmSampleCount, getLastPacketDuration()),
        1);
}

// re-construct the packet previous to the received packet
int32_t OpusDecoder::conceal(const unsigned char* payloadStart,
    int32_t payloadLength,
    int16_t* decodedData,
    const size_t pcmSampleCount)
{
    if (!_initialized)
    {
        assert(false);
        return -1;
    }
    assert(_state->_state);

    return opus_decode(_state->_state,
        payloadStart,
        payloadLength,
        reinterpret_cast<int16_t*>(decodedData),
        std::min<int32_t>(pcmSampleCount, getLastPacketDuration()),
        1);
}

int32_t OpusDecoder::getLastPacketDuration()
{
    if (!_initialized)
    {
        assert(false);
        return -1;
    }

    int32_t lastPacketDuration = 0;
    opus_decoder_ctl(_state->_state, OPUS_GET_LAST_PACKET_DURATION(&lastPacketDuration));
    return lastPacketDuration;
}

void OpusDecoder::onUnusedPacketReceived(uint32_t extendedSequenceNumber)
{
    const auto advance = static_cast<int32_t>(extendedSequenceNumber - _sequenceNumber);
    if (advance > 0)
    {
        _sequenceNumber = extendedSequenceNumber;
    }
}

int32_t OpusDecoder::decodePacket(const uint32_t extendedSequenceNumber,
    const unsigned char* payload,
    const size_t payloadLength,
    int16_t* audioData,
    const size_t maxPcmSamples)
{
    uint32_t samplesProduced = 0u;

    if (hasDecoded() && extendedSequenceNumber != getExpectedSequenceNumber())
    {
        const int32_t lossCount = static_cast<int32_t>(extendedSequenceNumber - getExpectedSequenceNumber());
        if (lossCount <= 0)
        {
            return 0;
        }

        const auto lastSampleCount = getLastPacketDuration();
        if (lastSampleCount <= 0)
        {
            return -1;
        }

        const size_t bufferPacketcountCapacity = maxPcmSamples / lastSampleCount;
        const auto concealCount = std::min(std::min<uint32_t>(2u, bufferPacketcountCapacity),
            extendedSequenceNumber - getExpectedSequenceNumber() - 1);
        for (uint32_t i = 0; concealCount > 1 && i < concealCount - 1; ++i)
        {
            const auto decodedFrames = conceal(audioData, maxPcmSamples - samplesProduced);
            if (decodedFrames > 0)
            {
                audioData += Opus::channelsPerFrame * decodedFrames;
                samplesProduced += decodedFrames;
            }
        }

        const auto decodedFrames = conceal(payload, payloadLength, audioData, maxPcmSamples - samplesProduced);
        if (decodedFrames > 0)
        {
            audioData += Opus::channelsPerFrame * decodedFrames;
            samplesProduced += decodedFrames;
        }
    }

    const auto decodedFrames = decode(extendedSequenceNumber,
        payload,
        payloadLength,
        reinterpret_cast<uint8_t*>(audioData),
        maxPcmSamples - samplesProduced);

    if (decodedFrames > 0)
    {
        audioData += Opus::channelsPerFrame * decodedFrames;
        samplesProduced += decodedFrames;
    }

    return samplesProduced;
}

} // namespace codec
