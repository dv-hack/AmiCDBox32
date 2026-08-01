// ISO9660Parser.cpp
//
#include "ISO9660Parser.hpp"

#include <cstring>
#include <iostream>

// ------------------------------------------------------------------------------------------------
std::string Iso9660Parser::getVolumeName(const Disc& disc)
{
    uint8_t sector[ SectorSize::ISO ];
    
    // Leggiamo il Primary Volume Descriptor (LBA 16)
    if (!disc.readSectors(16, 1, SectorSize::ISO, sector))
        return "";

    // Verifica che sia un PVD valido (Tipo 1, ID "CD001")
    if (sector[0] != 1 || std::memcmp(&sector[1], "CD001", 5) != 0)
        return ""; 

    // Il Volume Identifier inizia all'offset 40 ed è lungo 32 byte (spaziati alla fine)
    char volName[33] = {0};
    std::memcpy(volName, &sector[40], 32);
    
    std::string name(volName);
    
    // Rimuove gli spazi vuoti alla fine (padding ISO9660)
    size_t lastChar = name.find_last_not_of(' ');
    if (lastChar != std::string::npos)
        name.erase(lastChar + 1);
    
    return name;
}

// ------------------------------------------------------------------------------------------------
std::vector<IsoNode> Iso9660Parser::getRootDirectory(const Disc& disc)
{
    std::vector<IsoNode> files;
    uint8_t sector[SectorSize::ISO];

    // Leggiamo di nuovo il PVD per trovare la Root Directory
    if (!disc.readSectors(16, 1, SectorSize::ISO, sector))
        return files;
    
    if (sector[0] != 1 || std::memcmp(&sector[1], "CD001", 5) != 0)
        return files;

    // Il record della Root Directory si trova all'offset 156.
    // L'LBA di partenza è a +2, la dimensione in byte è a +10
    uint32_t rootLba = readLE32(&sector[158]);
    uint32_t rootSize = readLE32(&sector[166]);

    // Calcoliamo quanti settori occupa la directory e li leggiamo tutti
    uint32_t sectorsToRead = (rootSize + SectorSize::ISO-1 ) / SectorSize::ISO;
    std::vector<uint8_t> dirData(sectorsToRead * SectorSize::ISO);
    
    if (!disc.readSectors(rootLba, sectorsToRead, SectorSize::ISO, dirData.data()))
        return files;

    size_t offset = 0;
    while (offset < rootSize) 
    {
        uint8_t recordLen = dirData[offset];
        
        // Se la lunghezza è 0, significa che il settore ha finito i record.
        // Nel formato ISO9660 i record non cavallicano i settori: passiamo al settore successivo.
        if (recordLen == 0)
        {
            offset = ((offset / SectorSize::ISO) + 1) * SectorSize::ISO;
            continue;
        }

        uint32_t fileLba  = readLE32(&dirData[offset + 2]);
        uint32_t fileSize = readLE32(&dirData[offset + 10]);
        bool isDir        = (dirData[offset + 25] & 0x02) != 0; // Bit 1 = Directory flag
        uint8_t nameLen   = dirData[offset + 32];
        
        std::string fileName(reinterpret_cast<char*>(&dirData[offset + 33]), nameLen);

        // I file ISO9660 spesso terminano con ";1" (versione file). Lo rimuoviamo per pulizia.
        size_t versionPos = fileName.find(';');
        if (versionPos != std::string::npos) {
            fileName = fileName.substr(0, versionPos);
        }

        // Ignoriamo le directory speciali "." e ".." (in ISO9660 sono salvate come byte 0x00 e 0x01)
        if (nameLen == 1 && (fileName[0] == 0x00 || fileName[0] == 0x01)) {
            offset += recordLen;
            continue;
        }

        files.push_back({fileName, isDir, fileLba, fileSize});
        offset += recordLen;
    }

    return files;
}