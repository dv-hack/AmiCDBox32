// Disc.cpp
//
#include "Disc.hpp"

#include <iostream>

// ------------------------------------------------------------------------------------------------
uint32_t Disc::totalSectors() const 
{
    if (_tracks.empty()) return 0;
    const auto& lastTrack = _tracks.back();
    return lastTrack.startLBA() + lastTrack.sectorCount();
}

// ------------------------------------------------------------------------------------------------
bool Disc::readSectors(uint32_t lba, uint16_t blockCount, const SectorSize &sectorSize, uint8_t* buffer, uint32_t maxBufferSize) const
{
#if defined DEBUG
    std::cout << "[READ Sector]"
              << "  LBA:          " << lba << "\n"
              << "  blockCount:   " << blockCount << "\n"
              << "  sectorSize:   " << sectorSize << "\n"
              << "  maxBufferSize:" << maxBufferSize << "\n"
              ;
#endif

    // Guardrail: se i dati richiesti superano la RAM del buffer, abortiamo subito
    if ((uint32_t)blockCount * sectorSize > maxBufferSize)
        return false; 

    for (uint16_t i = 0; i < blockCount; ++i)
    {
        uint32_t currentLba = lba + i;
        Track* targetTrack = nullptr;

        // look for the track that contains this LBA
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