#include "G711codec.h"
#include "AudioTools.h"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <stdio.h>

namespace codec
{
int16_t PcmaDecoder::_table[256] = {0};
int8_t PcmaEncoder::_encodeTable[2048] = {0};

int16_t PcmuDecoder::_table[256] = {0};
uint8_t PcmuEncoder::_encodeTable[128] = {0};

PcmaDecoder::PcmaDecoder() : _audioSamplesPerPacket(160) {}

// TODO PLC if seqno has gap
int32_t PcmaDecoder::decodePacket(uint32_t extendedSequenceNumber,
    const unsigned char* payload,
    size_t payloadLength,
    int16_t* audioData,
    size_t audioBufferFrames)
{
    assert(_table[0] != 0);
    assert(audioBufferFrames >= payloadLength * 6);
    const uint32_t samples = payloadLength;
    if (samples * 6 < audioBufferFrames)
    {
        return 0;
    }

    int16_t pcmMono8k[samples];

    for (size_t i = 0; i < samples; ++i)
    {
        pcmMono8k[i] = _table[payload[i]];
    }

    _upSampler.upsample(pcmMono8k, audioData, samples);
    codec::makeStereo(audioData, samples);

    _audioSamplesPerPacket = samples;
    return samples * 6;
}

void PcmaDecoder::onUnusedPacketReceived(uint32_t extendedSequenceNumber) {}

int32_t PcmaDecoder::conceal(int16_t* audioData, size_t audioBufferFrames)
{
    std::memset(audioData, 0, _audioSamplesPerPacket * sizeof(int16_t) * 2);
    return _audioSamplesPerPacket;
}

void PcmaDecoder::initialize()
{
    for (int16_t d = 0; d < 256; ++d)
    {
        // unmask 0x55 and remove sign bit
        const int16_t ix = (d ^ 0x0055) & 0x007F;
        const int16_t exponent = ix >> 4;
        int16_t mant = ix & 0x000F;
        if (exponent > 0)
        {
            mant = mant + 16; /* add leading '1', if exponent > 0 */
        }

        mant = (mant << 4) + 8; /* now mantissa left adjusted and */
        /* 1/2 quantization step added */
        if (exponent > 1)
        {
            mant = mant << (exponent - 1);
        }

        _table[d] = d > 127 ? mant : -mant;
    }
}

PcmaEncoder::PcmaEncoder() {}

int32_t PcmaEncoder::encode(const int16_t* pcm,
    const size_t frames,
    unsigned char* payloadStart,
    const size_t payloadMaxFrames)
{
    assert(frames % 6 == 0);
    int16_t mono[frames];
    codec::makeMono(pcm, mono, frames, 0.5);

    const auto g711FrameCount = frames / 6;
    int16_t mono8k[g711FrameCount];
    _downSampler.downsample(mono, mono8k, frames);

    for (size_t n = 0; n < g711FrameCount; n++)
    {
        if (mono8k[n] < 0)
        {
            payloadStart[n] = _encodeTable[(~mono8k[n]) >> 4] ^ 0x0055;
        }
        else
        {
            payloadStart[n] = (_encodeTable[mono8k[n] >> 4] | 0x80) ^ 0x0055;
        }
    }

    return g711FrameCount;
}

void PcmaEncoder::initialize()
{
    for (int16_t d = 0; d < 2048; ++d)
    {
        int16_t x = d;
        if (x > 32) // exponent=0 for x <= 32
        {
            int16_t exponent = 1;
            while (x > 16 + 15) // find mantissa and exponent
            {
                x >>= 1;
                exponent++;
            }
            x -= 16; // second step: remove leading '1'
            x += exponent << 4; // now compute encoded value
        }

        _encodeTable[d] = x;
    }
}

PcmuDecoder::PcmuDecoder() : _audioSamplesPerPacket(160) {}

int32_t PcmuDecoder::decodePacket(uint32_t extendedSequenceNumber,
    const unsigned char* payload,
    size_t payloadLength,
    int16_t* audioData,
    size_t audioBufferFrames)
{
    assert(_table[0] != 0);
    assert(audioBufferFrames >= payloadLength * 6);
    const uint32_t samples = payloadLength;
    if (samples * 6 < audioBufferFrames)
    {
        return 0;
    }

    int16_t pcmMono8k[samples];
    for (size_t i = 1; i <= samples; ++i)
    {
        pcmMono8k[samples - i] = _table[payload[samples - i]];
    }
    _upSampler.upsample(pcmMono8k, audioData, samples);
    codec::makeStereo(audioData, samples * 6);
    _audioSamplesPerPacket = samples;
    return samples * 6;
}

int32_t PcmuDecoder::conceal(int16_t* audioData, size_t audioBufferFrames)
{
    std::memset(audioData, 0, _audioSamplesPerPacket * sizeof(int16_t) * 2);
    return _audioSamplesPerPacket;
}

void PcmuDecoder::onUnusedPacketReceived(uint32_t extendedSequenceNumber) {}

void PcmuDecoder::initialize()
{
    for (int16_t d = 0; d < 256; ++d)
    {
        // sign-bit = 1 for positiv values
        const int16_t sign = d < 0x0080 ? -1 : 1;
        int16_t mantissa = ~d; // 1's complement of input value
        const int16_t exponent = (mantissa >> 4) & 0x0007; // extract exponent
        const int16_t segment = exponent + 1; // compute segment number
        mantissa = mantissa & 0x000F; // extract mantissa

        // Compute Quantized Sample (14 bit left adjusted)
        int16_t step = 4 << segment; // position of the LSB = 1 quantization step)
        _table[d] = sign *
            ((0x0080 << exponent) // '1', preceding the mantissa
                + step * mantissa // left shift of mantissa
                + step / 2 // 1/2 quantization step
                - 4 * 33);
    }
}

PcmuEncoder::PcmuEncoder() {}

int32_t PcmuEncoder::encode(const int16_t* pcm16Stereo,
    const size_t frames,
    unsigned char* target,
    const size_t payloadMaxFrames)
{
    int16_t mono[frames];
    codec::makeMono(pcm16Stereo, mono, frames, 0.5);

    const auto g711Frames = frames / 6;
    int16_t pcm[g711Frames];
    _downSampler.downsample(mono, pcm, frames);

    // Change from 14 bit left adjusted to 14 bit right adjusted
    // Compute absolute value; adjust for easy processing
    for (size_t n = 0; n < g711Frames; ++n)
    {
        // compute 1's complement in case of  neg values. NB: 33 is the difference value
        int16_t absno = pcm[n] < 0 ? ((~pcm[n]) >> 2) + 33 : (pcm[n] >> 2) + 33;

        if (absno > 0x1FFF) // limit to "absno" < 8192
        {
            absno = 0x1FFF;
        }

        const int16_t segno = _encodeTable[absno >> 6]; // Determination of sample's segment

        // Mounting the high-nibble of the log-PCM sample
        const int16_t high_nibble = 0x0008 - segno;

        // Mounting the low-nibble of the log PCM sample
        // right shift of mantissa and masking away leading '1'
        int16_t low_nibble = (absno >> segno) & 0x000F;
        low_nibble = 0x000F - low_nibble;

        // Joining the high-nibble and the low-nibble of the log PCM sample
        target[n] = (high_nibble << 4) | low_nibble;

        // Add sign bit
        if (pcm[n] >= 0)
        {
            target[n] = target[n] | 0x0080;
        }
    }

    return g711Frames;
}

void PcmuEncoder::initialize()
{
    for (int16_t d = 0; d < 128; ++d)
    {
        int16_t i = d;
        int16_t segno = 1;
        while (i != 0)
        {
            segno++;
            i >>= 1;
        }
        _encodeTable[d] = segno;
    }
}

} // namespace codec
