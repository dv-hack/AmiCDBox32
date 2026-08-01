// VirtualNavDisc.cpp
//
#include "VirtualNavDisc.hpp"

#include "Endianess.hpp"

#include <cstring>
#include <algorithm>


#pragma region "helpers"

struct SdEntry 
{
    std::string name;
    bool isDirectory{false};
    uint32_t sizeBytes;
    uint32_t startLba; // LBA fittizio o reale se puntiamo a un file .iso
};

// ------------------------------------------------------------------------------------------------
static inline uint32_t generateDirectoryRecord(uint8_t* buffer, uint32_t lba, uint32_t size, bool isDir, const std::string& name)
{
    std::string isoName = name;
    // Sanitizzazione nome per ISO9660 semplice (tutto in maiuscolo)
    std::transform(isoName.begin(), isoName.end(), isoName.begin(), ::toupper);

    uint8_t nameLen = static_cast<uint8_t>(isoName.length());
    
    // Dimensione del record: 33 byte fissi di header + lunghezza nome
    uint8_t recordLen = 33 + nameLen;
    
    // Se la lunghezza totale è dispari, ISO 9660 impone 1 byte di padding finale per allineamento a 2
    if (recordLen % 2 != 0) 
        recordLen++;

    std::memset(buffer, 0, recordLen);

    buffer[0] = recordLen;                       // Offset 0: Record Length
    buffer[1] = 0;                                // Offset 1: Ext Attr Length
    Endianess::writeLB32(&buffer[2], lba);       // Offset 2..9: LBA
    Endianess::writeLB32(&buffer[10], size);     // Offset 10..17: Dimensione file in byte
    
    // Offset 18..24: Data/Ora fittizia (AA, MM, GG, HH, MM, SS, GMT Offset)
    buffer[18] = 126; // Anno dal 1900 (2026 -> 126)
    buffer[19] = 1;   // Gennaio
    buffer[20] = 1;   // Giorno 1
    
    buffer[25] = isDir ? 0x02 : 0x00;             // Offset 25: Flags (0x02 = Directory)
    Endianess::writeLB16(&buffer[28], 1);        // Offset 28..31: Volume Sequence Number (1)
    buffer[32] = nameLen;                         // Offset 32: Name Length

    // Offset 33+: Nome file / directory
    std::memcpy(&buffer[33], isoName.data(), nameLen);

    return recordLen;
}

// ------------------------------------------------------------------------------------------------
static inline void generatePvdSector(const std::string& volumeName, uint32_t rootDirSize, std::vector<uint8_t>& pvdSector)
{
    std::fill(pvdSector.begin(), pvdSector.end(), 0);

    pvdSector[0] = 1;                                     // Descriptor Type 1 (PVD)
    std::memcpy(&pvdSector[1], "CD001", 5);               // Standard Identifier
    pvdSector[6] = 1;                                     // Version 1

    // Offset 40..71: Volume Identifier (32 byte padded con spazi ASCII 0x20)
    std::memset(&pvdSector[40], ' ', 32);
    size_t copyLen = std::min(volumeName.length(), size_t(32));
    std::memcpy(&pvdSector[40], volumeName.c_str(), copyLen);

    // Volume Space Size (totale settori del disco)
    Endianess::writeLB32(&pvdSector[80], 18); // LBA 0-15 (System Area) + 16 (PVD) + 17 (Root Dir)
    
    // Volume Set Size & Sequence Number
    Endianess::writeLB16(&pvdSector[120], 1);
    Endianess::writeLB16(&pvdSector[124], 1);
    
    // Logical Block Size (2048 byte)
    Endianess::writeLB16(&pvdSector[128], SectorSize::ISO);

    // Root Directory Record incorporato dentro il PVD (all'offset 156)
    generateDirectoryRecord(&pvdSector[156], 17, rootDirSize, true, "\x00");
}

// ------------------------------------------------------------------------------------------------
static inline void buildFromEntries(const std::vector<SdEntry>& entries, const std::string& volumeName, std::vector<uint8_t>& pvdSector, std::vector<uint8_t>& rootDirSector)
{
    std::fill(rootDirSector.begin(), rootDirSector.end(), 0);
    uint32_t offset = 0;

    // 1. Record speciale "." (Current Directory)
    offset += generateDirectoryRecord(&rootDirSector[offset], 17, SectorSize::ISO, true, "\x00");

    // 2. Record speciale ".." (Parent Directory)
    offset += generateDirectoryRecord(&rootDirSector[offset], 17, SectorSize::ISO, true, "\x01");

    // 3. Aggiungiamo tutti i file e le cartelle trovati nella SD Card
    for (const auto& entry : entries) 
    {
        uint32_t recLen = generateDirectoryRecord(&rootDirSector[offset], entry.startLba, entry.sizeBytes, entry.isDirectory, entry.name);
        
        // Se sforiamo il settore da 2048 byte, ci fermiamo (o andrebbe allocato un secondo settore)
        if (offset + recLen > SectorSize::ISO)
            break;
        offset += recLen;
    }

    // const uint32_t totalSectors = 18;
    // _tracks[0]._sectorCount = totalSectors;

    // 4. Generiamo il Primary Volume Descriptor (LBA 16) agganciato alla Root Directory appena creata
    generatePvdSector(volumeName, offset, pvdSector);
}


#pragma endregion


// ------------------------------------------------------------------------------------------------
VirtualNavDisc::VirtualNavDisc()
    : _pvdSector(SectorSize::ISO, 0)
    , _rootDirSector(SectorSize::ISO, 0)
{
    const uint32_t totalSectors{18}; // Minimo LBA 0-15 (System Area) + 16 (PVD) + 17 (Root Dir)


    Track track;
    track._number = 1;
    track._startLBA = 0;
    track._sectorCount = totalSectors;
    track._type = Track::Type::Data; // Data Track Mode 1

    _tracks.push_back( std::move(track) );
}

VirtualNavDisc::VirtualNavDisc( const FileSystem::Path &rootPath, const std::string& volumeName )
 : VirtualNavDisc()
{
    // Leggiamo la lista di file e cartelle dalla SD Card
    std::vector<SdEntry> entries;

    FileSystem::Dir dir = FileSystem::openDir(rootPath.string());
    while ( dir.hasNext() )
    {
        FileSystem::FileInfo entry;
        dir.readNext(entry);

        SdEntry sdEntry;
        sdEntry.name = entry.name;
        sdEntry.isDirectory = entry.isDirectory;
        sdEntry.sizeBytes = entry.size;
        sdEntry.startLba = 18; // LBA fittizio per i file/cartelle della SD
        entries.push_back(sdEntry);
    }

    buildFromEntries(entries, volumeName, _pvdSector, _rootDirSector);
}

// ------------------------------------------------------------------------------------------------
bool VirtualNavDisc::readSectors(uint32_t lba, uint16_t count, const SectorSize &sectorSize, uint8_t* outBuffer) const
{
    for (uint32_t i = 0; i < count; ++i) 
    {
        uint32_t currentLba = lba + i;
        uint8_t* dest = outBuffer + (i * SectorSize::ISO);

        if (currentLba < 16) {
            // LBA 0-15: System Area (riempita a zero)
            std::memset(dest, 0, SectorSize::ISO);
        } else if (currentLba == 16) {
            // LBA 16: Primary Volume Descriptor
            std::memcpy(dest, _pvdSector.data(), SectorSize::ISO);
        } else if (currentLba == 17) {
            // LBA 17: Root Directory Data
            std::memcpy(dest, _rootDirSector.data(), SectorSize::ISO);
        } else {
            // Settori fuori limite o vuoti
            std::memset(dest, 0, SectorSize::ISO);
        }
    }
    return true;
}