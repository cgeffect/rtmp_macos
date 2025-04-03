#pragma once

#include <cstdint>
namespace tmms
{
    namespace mm
    {
        class NalBitStream 
        {
        public:
            NalBitStream(const char *data, int len);
            uint8_t GetBit();
            uint16_t GetWord(int bits);
            uint32_t GetBitLong(int bits);
            uint64_t GetBit64(int bits);
            uint32_t GetUE();
            int32_t GetSE();
        private:
            char GetByte();
            const char * data_;
            int len_;
            int bits_count_;
            int byte_idx_;
            char byte_;
        };
    }
}