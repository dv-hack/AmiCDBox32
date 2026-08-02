// Disc.hpp
//
#pragma once

#include "Track.hpp"
#include "Sector.hpp"

#include <vector>
#include <string>


class Disc
{
public:
    Disc() = default;
    virtual ~Disc() = default;

    virtual bool     readSectors(uint32_t lba, uint16_t blockCount, const SectorSize &sectorSize, uint8_t* buffer) const;
    uint32_t totalSectors() const;

    bool         isEmpty()         const  { return _tracks.empty(); }
    uint32_t     trackCount()      const  { return (uint32_t)_tracks.size();} 
    Track&       track(uint32_t i)        { return _tracks[i];}
    const Track& track(uint32_t i) const  { return _tracks[i];}

protected:
    std::vector<Track> _tracks;

    friend class Mounter;
};