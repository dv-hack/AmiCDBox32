// Track.cpp
//
#include "Track.hpp"

#include <iostream>

// ------------------------------------------------------------------------------------------------
Track Track::makeISOTrack( const std::string &filename )
{
    auto file = FileSystem::open(filename);
    if (!file.isOpen())
    {
        std::cout << "[makeISOTrack] Error: unable to open " << filename << "\n";
        return {};
    }

    // compute sectors. iso files have 2048bytes sectors
    const uint32_t fileSize = FileSystem::stat( filename ).size;
    const uint32_t sectorSize = static_cast<uint32_t>( SectorSize::ISO);
    const uint32_t sectorCount = fileSize / sectorSize;

    if (fileSize % sectorSize != 0) // not multiple of 2048
        std::cout << "[Mounter] Warning: suspicious ISO size.\n";

    Track t;
    t._number = 1;
    t._sectorCount = sectorCount;
    t._sectorSize = sectorSize;
    t._startLBA = 0;
    t._type = Type::Data;
    t._file = std::move( file );

    return t;
}


// ------------------------------------------------------------------------------------------------
bool Track::readData(uint32_t relLBA, uint8_t *outBuffer)
{
    const auto offset = relLBA * _sectorSize;
    const auto ok = _file.seek(offset);
    if (ok != FileSystem::ErrorCode::NoError)
    {
        std::cout << "[Track] Error: seek failed. ErrorCode: " << static_cast<int>(ok) << '\n';
        return false;
    }

    if (_sectorSize == 2048)
    {
        uint32_t bytes{0};
        const auto readOk = _file.read(outBuffer, _sectorSize, bytes);
        if (readOk != FileSystem::ErrorCode::NoError)
        {
            std::cout << "[Track] Error: read error. ErrorCode: " << static_cast<int>(readOk) << '\n';
            return false;
        }
        return true;
    }

    // 1. Calcola l'offset nel file: relLba * sectorSize
    // 2. Esegui la seek(offset)
    // 3. Se sectorSize == 2048 -> f_read direttamente in outBuffer
    // 4. Se sectorSize == 2352 (Dati) -> f_read in un buffer temporaneo, copia i byte da [16] a [2063] in outBuffer
    return false;
}