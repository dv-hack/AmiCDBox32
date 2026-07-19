// Sector.hpp
//
#pragma once

#include <cstdint>

static constexpr int16_t kSectorSize = 2352;
struct Sector
{
    uint8_t bytes[kSectorSize];
};
