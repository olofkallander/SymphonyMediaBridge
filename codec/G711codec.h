#pragma once

#include "codec/AudioDecoder.h"
#include "codec/AudioEncoder.h"
#include "codec/ResampleFilters.h"
#include <cstddef>
#include <cstdint>

namespace codec
{

class PcmaDecoder : public AudioDecoder
{
public:
    PcmaDecoder();

    int32_t decodePacket(uint32_t extendedSequenceNumber,
        const unsigned char* payload,
        size_t payloadLength,
        int16_t* audioData,
        size_t audioBufferFrames) override;

    void onUnusedPacketReceived(uint32_t extendedSequenceNumber) override;

    int32_t conceal(int16_t* audioData, size_t audioBufferFrames) override;

    static void initialize();

private:
    static int16_t _table[256];

    uint32_t _audioSamplesPerPacket;
    codec::FirUpsample6x _upSampler;
};

class PcmaEncoder : public AudioEncoder
{
public:
    PcmaEncoder();

    int32_t encode(const int16_t* pcm16Stereo,
        const size_t frames,
        unsigned char* payloadStart,
        const size_t payloadMaxFrames) override;

    static void initialize();

private:
    static int8_t _encodeTable[2048];

    codec::FirDownsample6x _downSampler;
};

class PcmuDecoder : public AudioDecoder
{
public:
    PcmuDecoder();

    int32_t decodePacket(uint32_t extendedSequenceNumber,
        const unsigned char* payload,
        size_t payloadLength,
        int16_t* audioData,
        size_t audioBufferFrames) override;

    void onUnusedPacketReceived(uint32_t extendedSequenceNumber) override;

    int32_t conceal(int16_t* audioData, size_t audioBufferFrames) override;

    static void initialize();

private:
    static int16_t _table[256];

    uint32_t _audioSamplesPerPacket;
    codec::FirUpsample6x _upSampler;
};

class PcmuEncoder : public AudioEncoder
{
public:
    PcmuEncoder();

    int32_t encode(const int16_t* pcm16Stereo,
        const size_t frames,
        unsigned char* payloadStart,
        const size_t payloadMaxFrames) override;

    static void initialize();

private:
    static uint8_t _encodeTable[128];

    codec::FirDownsample6x _downSampler;
};

} // namespace codec
