#pragma once
#include "codec/OpusEncoder.h"
#include "memory/PacketPoolAllocator.h"
#include "test/bwe/FakeAudioSource.h"
#include <cstdint>
namespace emulator
{
enum Audio
{
    None = 0,
    Opus,
    Fake,
    Muted
};
class AudioSource : public fakenet::MediaSource
{
public:
    enum PttState
    {
        NotSpecified = 0,
        Unset,
        Set
    };

    AudioSource(memory::PacketPoolAllocator& allocator, uint32_t ssrc, Audio fakeAudio, uint32_t ptime = 20);
    ~AudioSource();
    memory::UniquePacket getPacket(uint64_t timestamp) override;
    int64_t timeToRelease(uint64_t timestamp) const override;

    memory::PacketPoolAllocator& getAllocator() { return _allocator; }

    uint32_t getSsrc() const override { return _ssrc; }

    void setBandwidth(uint32_t kbps) override;
    uint32_t getBandwidth() const override { return 122; }

    void setVolume(double normalized) { _amplitude = 15000 * normalized; }
    void setFrequency(double frequency) { _frequency = frequency; }
    void setPtt(const PttState isPtt);
    void setUseAudioLevel(const bool useAudioLevel);
    void enableIntermittentTone(double onRatio);

    bool openPcm16File(const char* filename);

    uint16_t getSequenceCounter() const { return _sequenceCounter; }

    void enablePacketPadding() { _padPackets = true; }

private:
    static const uint32_t maxSentBufferSize = 12 * 1024;
    uint32_t _ssrc;
    codec::OpusEncoder _encoder;
    uint64_t _nextRelease;
    memory::PacketPoolAllocator& _allocator;
    double _phase;
    uint32_t _rtpTimestamp;
    uint16_t _sequenceCounter;
    uint16_t _amplitude;
    double _frequency;
    uint32_t _ptime;
    PttState _isPtt;
    bool _useAudioLevel;
    Audio _emulatedAudioType;
    FILE* _pcm16File;
    uint32_t _packetCount;
    bool _padPackets;

    struct TonePattern
    {
        double onRatio = 1.0;
        int32_t silenceCountDown = 0;
    } _tonePattern;
};

} // namespace emulator
