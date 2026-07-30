// Endianess.hpp
//
#pragma once

#include <cstdint>

namespace Endianess
{
    inline uint32_t readBE32(const uint8_t* ptr) {
        return (static_cast<uint32_t>(ptr[0]) << 24) |
               (static_cast<uint32_t>(ptr[1]) << 16) |
               (static_cast<uint32_t>(ptr[2]) << 8)  |
               (static_cast<uint32_t>(ptr[3]));
    }

    inline uint16_t readBE16(const uint8_t* ptr) {
        return (static_cast<uint16_t>(ptr[0]) << 8) |
               (static_cast<uint16_t>(ptr[1]));
    }

    inline void writeBE32(uint8_t* ptr, uint32_t value) {
        ptr[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
        ptr[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        ptr[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        ptr[3] = static_cast<uint8_t>(value & 0xFF);
    }

    inline void writeBE16(uint8_t* ptr, uint16_t value) {
        ptr[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
        ptr[1] = static_cast<uint8_t>(value & 0xFF);
    }
}