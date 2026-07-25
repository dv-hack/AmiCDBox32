// Sector.hpp
//
#pragma once

#include <cstdint>
#include <vector>

//static constexpr int16_t kSectorSize = 2352; //cue?
//static constexpr int16_t kSectorSize = 2048; //iso?

struct Sector
{
    std::vector<uint8_t> bytes;
    int size = 0;
    
    explicit Sector( int reserve ) {bytes.reserve( reserve ); size = reserve;}
};