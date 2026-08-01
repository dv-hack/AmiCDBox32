#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "Disc.hpp"
#include "Sector.hpp"
#include "FileSystem.hpp"

class VirtualNavDisc : public Disc
{
public:
    VirtualNavDisc();
    VirtualNavDisc( const FileSystem::Path &rootPath, const std::string& volumeName = "SD_NAVIGATOR" );

    bool readSectors(uint32_t lba, uint16_t count, const SectorSize &sectorSize, uint8_t* outBuffer) const override;

private:
    std::vector<uint8_t> _pvdSector;     // Buffer per LBA 16 (2048 byte)
    std::vector<uint8_t> _rootDirSector; // Buffer per LBA 17 (2048 byte)
};