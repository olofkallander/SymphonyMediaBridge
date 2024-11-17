#include "codec/G711codec.h"
#include "codec/AudioLevel.h"
#include "codec/AudioTools.h"
#include "codec/ResampleFilters.h"
#include "logger/Logger.h"
#include "utils/ScopedFileHandle.h"
#include "utils/Time.h"
#include <cmath>
#include <gtest/gtest.h>

namespace codec
{
int computeAudioLevel(const int16_t* payload, int samples);
}

struct G711Test : testing::Test
{
    void SetUp() override
    {
        for (size_t i = 0; i < samples; ++i)
        {
            _pcmData[i] = sin(2 * PI * i * 400 / sampleFrequency) * amplitude;
        }
        codec::makeStereo(_pcmData, samples);

        codec::PcmaEncoder::initialize();
        codec::PcmuEncoder::initialize();
        codec::PcmaDecoder::initialize();
        codec::PcmuDecoder::initialize();
    }

    static constexpr double PI = 3.14159;
    static constexpr double sampleFrequency = 48000;
    static constexpr size_t samples = sampleFrequency / 50;
    static constexpr double amplitude = 2000;
    int16_t _pcmData[samples * 2]; // pcm16 stereo
};

TEST_F(G711Test, alaw)
{
    codec::PcmaEncoder encoder;
    codec::PcmaDecoder decoder;

    auto dB = codec::computeAudioLevel(_pcmData, samples * 2);

    const auto g711Samples = samples / 6;
    uint32_t seqNo = 34534;
    uint8_t g711[g711Samples];
    EXPECT_EQ(encoder.encode(_pcmData, samples, g711, g711Samples), g711Samples);

    auto framesProduced = decoder.decodePacket(seqNo, g711, g711Samples, _pcmData, samples);
    EXPECT_EQ(framesProduced, samples);
    auto dB2 = codec::computeAudioLevel(_pcmData, samples * 2);

    EXPECT_EQ(dB2, dB);
    EXPECT_EQ(static_cast<int>(27), dB);
}

TEST_F(G711Test, ulaw)
{
    codec::PcmuEncoder encoder;
    codec::PcmuDecoder decoder;

    auto dB = codec::computeAudioLevel(_pcmData, samples);

    const auto g711Samples = samples / 6;
    uint8_t g711[g711Samples];
    encoder.encode(_pcmData, samples, g711, g711Samples);

    decoder.decodePacket(23423, g711, g711Samples, _pcmData, samples);
    auto dB2 = codec::computeAudioLevel(_pcmData, samples);

    EXPECT_EQ(dB, dB2);
    EXPECT_EQ(static_cast<int>(27), dB);
}

TEST_F(G711Test, DISABLED_alawFrom48k)
{
    utils::ScopedFileHandle inFile(::fopen("../tools/testfiles/jpsample.raw", "r"));
    utils::ScopedFileHandle outFile(::fopen("pcma8kout.raw", "wr"));

    codec::FirDownsample6x downSampler;
    codec::PcmaEncoder encoder;

    int16_t samples48k[960 * 2];
    for (auto i = 0u; i < 500; ++i)
    {
        ::fread(samples48k, sizeof(int16_t), 960, inFile.get());
        codec::makeStereo(samples48k, 960);

        uint8_t pcma[160];
        encoder.encode(samples48k, 960, pcma, 160);
        ::fwrite(pcma, sizeof(uint8_t), 160, outFile.get());
    }
}

TEST_F(G711Test, DISABLED_ulawFrom48k)
{
    utils::ScopedFileHandle inFile(::fopen("../tools/testfiles/jpsample.raw", "r"));
    utils::ScopedFileHandle outFile(::fopen("pcmu8kout.raw", "wr"));

    codec::FirDownsample6x downSampler;
    codec::PcmaEncoder encoder;

    int16_t samples48k[960 * 2];
    for (auto i = 0u; i < 500; ++i)
    {
        ::fread(samples48k, sizeof(int16_t), 960, inFile.get());
        codec::makeStereo(samples48k, 960);

        uint8_t pcma[160];
        encoder.encode(samples48k, 960, pcma, 160);
        ::fwrite(pcma, sizeof(uint8_t), 160, outFile.get());
    }
}
