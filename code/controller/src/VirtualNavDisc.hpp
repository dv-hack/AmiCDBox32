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
    struct IsoNode
    {
        std::string isoName;
        std::string sdPath;
        bool isDirectory;
        uint32_t sizeBytes;

        uint32_t startLba;
        uint32_t sectorCount;

        uint16_t dirId;
        uint16_t parentDirId;

        std::vector<uint8_t> dirData;         // Conterrà i 2048 byte del record directory pre-generato
    };

public:
    VirtualNavDisc();
    VirtualNavDisc( const FileSystem::Path &rootPath, const std::string& volumeName = "SD_NAVIGATOR" );

    bool readSectors(uint32_t lba, uint16_t count, const SectorSize &sectorSize, uint8_t* outBuffer, uint32_t maxBufferSize ) const override;

private:
    void discoverNodes(const std::string& path, uint16_t parentId, uint32_t& currentLba, uint16_t& dirIdCounter);

private:
    std::vector<uint8_t> _pvdSector;     // Buffer per LBA 16 (2048 byte)
    std::vector<uint8_t> _rootDirSector; // Buffer per LBA 17 (2048 byte)
    std::vector<IsoNode> _isoTree;
    std::vector<uint8_t> _pathTableL;
    std::vector<uint8_t> _pathTableM;
};