// Track.cpp
//
#include "Track.hpp"

#include "Sector.hpp"

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
    const uint32_t sectorSize = SectorSize::ISO;
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
    const auto offset = (relLBA + _fileSectorOffset) * _sectorSize;
    const auto ok = _file.seek(offset);
    if (ok != FileSystem::ErrorCode::NoError)
    {
        std::cout << "[Track] Error: seek failed. ErrorCode: " << static_cast<int>(ok) << '\n';
        return false;
    }

    if (_sectorSize == SectorSize::ISO )
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

    if( _sectorSize == SectorSize::RAW )
    {
        uint8_t tempBuffer[SectorSize::RAW];
        uint32_t bytes{0};
        const auto readOk = _file.read(tempBuffer, _sectorSize, bytes);
        if (readOk != FileSystem::ErrorCode::NoError || bytes != _sectorSize)
        {
            std::cout << "[Track] Error: read error. ErrorCode: " << static_cast<int>(readOk) << " bytes: " << bytes << '\n';
            return false;
        }

        if     (_type == Type::Audio) std::copy(tempBuffer, tempBuffer + 2352, outBuffer);              // Copy the audio portion (2352 bytes) to outBuffer
        else if(_type == Type::Data)  std::copy(tempBuffer + 16, tempBuffer + 16 + 2048, outBuffer);   // Copy the data portion (2048 bytes) to outBuffer
        else
        {
            std::cout << "[Track] Error: unknown track type.\n";
            return false;
        }
        return true;
    }

    return false;
}