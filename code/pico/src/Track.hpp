// Track.hpp
//
#pragma once

#include "FileSystem.hpp"

#include <cstdint>

class Track
{
public:
    enum class Type { Data, Audio };
    enum class SectorSize { ISO = 2048, RAW = 2352 };

    static Track makeISOTrack( const std::string &filename );

public:
    Track() = default;
    // Track(uint8_t num, uint32_t startLBA, uint32_t sectorCount, uint16_t sectorSize, Type t, FileSystem::File f)
    //     : _number(num)
    //     , _startLBA(startLBA)
    //     , _sectorCount(sectorCount)
    //     , _sectorSize(sectorSize)
    //     , _type(t)
    //     , _file( std::move(f) )
    // {}


    uint8_t   number()      const { return _number; }
    uint32_t  startLBA()    const { return _startLBA; }
    uint32_t  sectorCount() const { return _sectorCount; }
    uint16_t  sectorSize()  const { return _sectorSize; }
    Type      type()        const { return _type; }

    bool readData(uint32_t relLBA, uint8_t* outBuffer);


private:
    uint8_t   _number     {0};
    uint32_t  _startLBA   {0};
    uint32_t  _sectorCount{0};
    uint16_t  _sectorSize {0}; // 2048 (ISO), 2352 (BIN/RAW)
    Type      _type { Type::Data};

    FileSystem::File _file;

};