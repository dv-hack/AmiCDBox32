// VirtualNavDisc.cpp
//
#include "VirtualNavDisc.hpp"

#include "Endianess.hpp"

#include <cstring>
#include <algorithm>

#pragma region helpers
static inline uint32_t appendPathTableEntry(uint8_t *buffer, uint32_t offset, const VirtualNavDisc::IsoNode &dirNode, bool isBigEndian)
{
    uint8_t nameLen = (dirNode.dirId == 1) ? 1 : static_cast<uint8_t>(dirNode.isoName.length());

    buffer[offset] = nameLen; // LEN_DI
    buffer[offset + 1] = 0;   // Ext Attr Record Length

    if (isBigEndian)
    {
        buffer[offset + 2] = (dirNode.startLba >> 24) & 0xFF;
        buffer[offset + 3] = (dirNode.startLba >> 16) & 0xFF;
        buffer[offset + 4] = (dirNode.startLba >> 8) & 0xFF;
        buffer[offset + 5] = dirNode.startLba & 0xFF;

        buffer[offset + 6] = (dirNode.parentDirId >> 8) & 0xFF;
        buffer[offset + 7] = dirNode.parentDirId & 0xFF;
    }
    else
    {
        buffer[offset + 2] = dirNode.startLba & 0xFF;
        buffer[offset + 3] = (dirNode.startLba >> 8) & 0xFF;
        buffer[offset + 4] = (dirNode.startLba >> 16) & 0xFF;
        buffer[offset + 5] = (dirNode.startLba >> 24) & 0xFF;

        buffer[offset + 6] = dirNode.parentDirId & 0xFF;
        buffer[offset + 7] = (dirNode.parentDirId >> 8) & 0xFF;
    }

    if (dirNode.dirId == 1)
    {
        buffer[offset + 8] = 0x00; // La Root si chiama sempre 0x00
    }
    else
    {
        std::memcpy(&buffer[offset + 8], dirNode.isoName.c_str(), nameLen);
    }

    uint32_t totalLen = 8 + nameLen;
    // Padding: se la lunghezza del nome è dispari, aggiungi 1 byte a zero
    if (nameLen % 2 != 0)
    {
        buffer[offset + totalLen] = 0;
        totalLen++;
    }
    return totalLen;
}

// ------------------------------------------------------------------------------------------------
static inline uint32_t generateDirectoryRecord(uint8_t *buffer, uint32_t lba, uint32_t size, bool isDir, const std::string &name)
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

    buffer[0] = recordLen;                   // Offset 0: Record Length
    buffer[1] = 0;                           // Offset 1: Ext Attr Length
    Endianess::writeLB32(&buffer[2], lba);   // Offset 2..9: LBA
    Endianess::writeLB32(&buffer[10], size); // Offset 10..17: Dimensione file in byte

    // Offset 18..24: Data/Ora fittizia (AA, MM, GG, HH, MM, SS, GMT Offset)
    buffer[18] = 126; // Anno dal 1900 (2026 -> 126)
    buffer[19] = 1;   // Gennaio
    buffer[20] = 1;   // Giorno 1

    buffer[25] = isDir ? 0x02 : 0x00;     // Offset 25: Flags (0x02 = Directory)
    Endianess::writeLB16(&buffer[28], 1); // Offset 28..31: Volume Sequence Number (1)
    buffer[32] = nameLen;                 // Offset 32: Name Length

    // Offset 33+: Nome file / directory
    std::memcpy(&buffer[33], isoName.data(), nameLen);

    return recordLen;
}

// ------------------------------------------------------------------------------------------------
static inline void generatePvdSector(const std::string &volumeName, uint32_t rootDirSize, uint32_t totalSectors, uint32_t pathTableSize, std::vector<uint8_t> &pvdSector)
{
    std::fill(pvdSector.begin(), pvdSector.end(), 0);

    pvdSector[0] = 1;                       
    std::memcpy(&pvdSector[1], "CD001", 5);   
    pvdSector[6] = 1;                       

    std::memset(&pvdSector[40], ' ', 32);
    size_t copyLen = std::min(volumeName.length(), size_t(32));
    std::memcpy(&pvdSector[40], volumeName.c_str(), copyLen);

    // 1. Volume Space Size REALE
    Endianess::writeLB32(&pvdSector[80], totalSectors); 
    
    Endianess::writeLB16(&pvdSector[120], 1);
    Endianess::writeLB16(&pvdSector[124], 1);
    Endianess::writeLB16(&pvdSector[128], SectorSize::ISO); 

    // 2. Path Table Size REALE
    Endianess::writeLB32(&pvdSector[132], pathTableSize); // Nota: su ISO9660 standard è un intero a 32bit nei campi di offset lunghi, ma per comodità usa la scrittura a 32 bit o scrivi sui byte corretti
    Endianess::writeLB32(&pvdSector[140], pathTableSize);

    // LBA Path Tables (17 e 18)
    pvdSector[140] = 17; // Type L LBA
    pvdSector[151] = 18; // Type M LBA

    const char dummyDate[] = "2024010112000000";
    std::memcpy(&pvdSector[813], dummyDate, 16); 
    std::memcpy(&pvdSector[830], dummyDate, 16); 

    generateDirectoryRecord(&pvdSector[156], 19, rootDirSize, true, std::string("\x00", 1));
}
#pragma endregion

// ------------------------------------------------------------------------------------------------
VirtualNavDisc::VirtualNavDisc()
    : _pvdSector(SectorSize::ISO, 0), _rootDirSector(SectorSize::ISO, 0)
{
    const uint32_t totalSectors{18}; // Minimo LBA 0-15 (System Area) + 16 (PVD) + 17 (Root Dir)

    Track track;
    track._number = 1;
    track._startLBA = 0;
    track._sectorCount = totalSectors;
    track._type = Track::Type::Data; // Data Track Mode 1

    _tracks.push_back(std::move(track));
}

// ------------------------------------------------------------------------------------------------
VirtualNavDisc::VirtualNavDisc(const FileSystem::Path &rootPath, const std::string &volumeName)
    : VirtualNavDisc()
{
    uint32_t currentLba = 20; // 0-15 Sys, 16 PVD, 17 PathL, 18 PathM, 19 Root
    uint16_t dirIdCounter = 1;

    // --- PASSAGGIO 1: Creazione Root e Scansione ---
    IsoNode root;
    root.isoName = "\x00";
    root.sdPath = rootPath.string();
    root.isDirectory = true;
    root.sizeBytes = SectorSize::ISO;
    root.startLba = 19;
    root.sectorCount = 1;
    root.dirId = 1;
    root.parentDirId = 1;
    _isoTree.push_back(root);

    discoverNodes(rootPath.string(), 1, currentLba, dirIdCounter);

    // --- PASSAGGIO 2: Costruzione in RAM ---
    _pathTableL.assign(SectorSize::ISO, 0);
    _pathTableM.assign(SectorSize::ISO, 0);
    uint32_t ptOffsetL = 0;
    uint32_t ptOffsetM = 0;

    for (auto &node : _isoTree)
    {
        if (node.isDirectory)
        {
            // A. Aggiungi la directory alle Path Table
            ptOffsetL += appendPathTableEntry(_pathTableL.data(), ptOffsetL, node, false);
            ptOffsetM += appendPathTableEntry(_pathTableM.data(), ptOffsetM, node, true);

            // B. Genera i record della directory (.) e (..)
            node.dirData.assign(SectorSize::ISO, 0);
            uint32_t offset = 0;

            // Directory Corrente (.)
            offset += generateDirectoryRecord(node.dirData.data() + offset, node.startLba, SectorSize::ISO, true, std::string("\x00", 1));

            // Directory Genitore (..) -> Cerca l'LBA del padre
            uint32_t parentLba = 19;
            if (node.dirId != 1)
            {
                for (const auto &p : _isoTree)
                {
                    if (p.dirId == node.parentDirId)
                    {
                        parentLba = p.startLba;
                        break;
                    }
                }
            }
            offset += generateDirectoryRecord(node.dirData.data() + offset, parentLba, SectorSize::ISO, true, std::string("\x01", 1));

            // C. Inserisci tutti i file e le cartelle figlie
            for (const auto &child : _isoTree)
            {
                if (child.parentDirId == node.dirId && child.dirId != node.dirId)
                {
                    uint32_t recLen = generateDirectoryRecord(node.dirData.data() + offset, child.startLba, child.sizeBytes, child.isDirectory, child.isoName);
                    offset += recLen;
                }
            }
        }
    }


    // Alla fine del costruttore VirtualNavDisc(...)
    generatePvdSector(volumeName, SectorSize::ISO, currentLba, ptOffsetL, _pvdSector);
    _tracks[0]._sectorCount = currentLba;
}

// ------------------------------------------------------------------------------------------------
void VirtualNavDisc::discoverNodes(const std::string &path, uint16_t parentId, uint32_t &currentLba, uint16_t &dirIdCounter)
{
    FileSystem::Dir dir = FileSystem::openDir(path);
    while (dir.hasNext())
    {
        FileSystem::FileInfo entry;
        dir.readNext(entry);

        IsoNode node;
        node.isoName = entry.name;
        std::transform(node.isoName.begin(), node.isoName.end(), node.isoName.begin(), ::toupper);
        node.sdPath = path + "/" + entry.name;
        node.isDirectory = entry.isDirectory;
        node.sizeBytes = entry.size;
        node.parentDirId = parentId;
        node.startLba = currentLba;

        if (node.isDirectory)
        {
            node.sectorCount = 1; // Limitiamo a 1 settore per dir (~60 file)
            dirIdCounter++;
            node.dirId = dirIdCounter;
            currentLba += node.sectorCount;

            _isoTree.push_back(node);

            // Chiamata ricorsiva per le sottocartelle
            discoverNodes(node.sdPath, node.dirId, currentLba, dirIdCounter);
        }
        else
        {
            node.sectorCount = (entry.size + (SectorSize::ISO - 1)) / SectorSize::ISO;
            node.dirId = 0;
            currentLba += node.sectorCount;

            _isoTree.push_back(node);
        }
    }
}

// ------------------------------------------------------------------------------------------------
bool VirtualNavDisc::readSectors(uint32_t lba, uint16_t count, const SectorSize &sectorSize, uint8_t *outBuffer, uint32_t maxBufferSize ) const
{
    (void) maxBufferSize;

    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t currentLba = lba + i;
        uint8_t *dest = outBuffer + (i * SectorSize::ISO);

        if (currentLba < 16)
        {
            // LBA 0-15: System Area (riempita a zero)
            std::memset(dest, 0, SectorSize::ISO);
        }
        else if (currentLba == 16)
        {
            // LBA 16: Primary Volume Descriptor
            std::memcpy(dest, _pvdSector.data(), SectorSize::ISO);
        }
        else if (currentLba == 17)
        {
            // LBA 17: Paht Table Type L (Little Endian)
            std::memcpy(dest, _pathTableL.data(), SectorSize::ISO);
        }
        else if (currentLba == 18)
        {
            // LBA 18: Type M Path Table (Big Endian)
            std::memcpy(dest, _pathTableM.data(), SectorSize::ISO);
        }
        else 
        {
            // Verifichiamo a quale nodo appartiene il primo settore
            const IsoNode* currentNode = nullptr;
            for (const auto &node : _isoTree) 
            {
                if (currentLba >= node.startLba && currentLba < (node.startLba + node.sectorCount)) 
                {
                    currentNode = &node;
                    break;
                }
            }

            if (!currentNode) 
            {
                std::memset(dest, 0, SectorSize::ISO);
            } 
            else if (currentNode->isDirectory) 
            {
                // Le directory sono in RAM, nessun problema di velocità
                std::memcpy(dest, currentNode->dirData.data(), SectorSize::ISO);
            } 
            else 
            {
                // È un file fisico: calcoliamo quanti settori consecutivi di QUESTO stesso file 
                // possiamo leggere in un unico colpo (batching basato su 'count')
                uint32_t fileSectorOffset = currentLba - currentNode->startLba;
                uint32_t remainingInFile = currentNode->sectorCount - fileSectorOffset;
                uint32_t sectorsToRead = std::min(static_cast<uint32_t>(count - i), remainingInFile);

                uint32_t byteOffset = fileSectorOffset * SectorSize::ISO;

                // Apriamo il file UNA SOLA VOLTA per il blocco contiguo
                auto file = FileSystem::open(currentNode->sdPath);
                if (file.isOpen()) 
                {
                    file.seek(byteOffset);
                    
                    uint32_t totalBytesToRead = sectorsToRead * SectorSize::ISO;
                    uint32_t bytesRead = 0;
                    
                    // Leggiamo tutto il blocco in un colpo solo direttamente nel buffer di destinazione
                    file.read(dest, totalBytesToRead, bytesRead);
                    
                    if (bytesRead < totalBytesToRead) {
                        std::memset(dest + bytesRead, 0, totalBytesToRead - bytesRead);
                    }
                    file.close();
                } 
                else 
                {
                    std::memset(dest, 0, sectorsToRead * SectorSize::ISO);
                }

                // Avanziamo l'indice del ciclo 'i' di quanti settori abbiamo appena letto in blocco
                i += (sectorsToRead - 1); 
            }
        }
    }
    return true;
}