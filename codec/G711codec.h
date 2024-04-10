#pragma once

#include "codec/AudioDecoder.h"
#include <cstddef>
#include <cstdint>

namespace codec
{

class PcmaCodec : public AudioDecoder
{
public:
    static void encode(const int16_t* data, uint8_t* target, size_t samples);
    static void decode(const uint8_t* data, int16_t* target, size_t samples);

    static void initialize();

private:
    static int16_t _table[256];
    static int8_t _encodeTable[2048];
};

class PcmuCodec : public AudioDecoder
{
public:
    static void encode(const int16_t* data, uint8_t* target, size_t samples);
    static void decode(const uint8_t* data, int16_t* target, size_t samples);

    static void initialize();

private:
    static int16_t _table[256];
    static uint8_t _encodeTable[128];
};

} // namespace codec
