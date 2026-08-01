// Disc.cpp
//
#include "Disc.hpp"


// ------------------------------------------------------------------------------------------------
uint32_t Disc::totalSectors() const 
{
    if (_tracks.empty()) return 0;
    const auto& lastTrack = _tracks.back();
    return lastTrack.startLBA() + lastTrack.sectorCount();
}

// ------------------------------------------------------------------------------------------------
bool Disc::readSectors(uint32_t lba, uint16_t blockCount, const SectorSize &sectorSize, uint8_t* buffer) const
{
    for (uint16_t i = 0; i < blockCount; ++i)
    {
        uint32_t currentLba = lba + i;
        Track* targetTrack = nullptr;

        // Trova la traccia che contiene questo LBA
        for ( auto& t : _tracks)
            if (currentLba >= t.startLBA() && currentLba < (t.startLBA() + t.sectorCount()))
            {
                targetTrack = const_cast<Track*>(&t);
                break;
            }

        if (!targetTrack) 
            return false; // LBA out of disc boundaries

        // compute LBA relative to target track
        uint32_t relLba = currentLba - targetTrack->startLBA();
        
        // read data from track into provided buffer
        if (!targetTrack->readData(relLba, buffer + (i * sectorSize)))
            return false; 
    }
    return true;
}