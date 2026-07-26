// CdMap.hpp
//
#pragma once

#include "BinFile.hpp"

#include <string>
#include <string_view>
#include <vector>

enum class TrackType
{
    Data, // Mode 1 data track (normally ISO-9660)
    Audio // CD-DA tracks (PCM 44.1kHz, 16-bit Stereo)
};

struct Track
{
    int number = 0;
    TrackType type = TrackType::Data;
    BinFile* binFile{nullptr};    // <-- OGNI TRACCIA HA IL SUO FILE .BIN
    size_t startSector = 0;       // LBA Assoluto sul CD virtuale (per la stringa di boot Amiga)
    size_t endSector = 0;         // LBA Assoluto di fine traccia
    size_t localOffsetSector = 0; // Offset interno al file specifico (spesso 150 per il pregap)
    uint32_t sectorSize = 0;
};

class Disc
{
public:
    static Disc mount(  std::string_view filename );

public:
    Disc();
    
    bool isValid() const { return !_rootFolder.empty() && !_tracks.empty(); }
    
    Track *getTrackAtSector(size_t sector)
    {
        for (auto &track : _tracks)
            if (sector >= track.startSector && (track.endSector == 0 || sector <= track.endSector))
                return &track;
        return nullptr;
    }
    
    uint32_t getTotalSectors() const;
private:
    std::string _rootFolder;
    std::string _mountedFile;
    std::vector<Track> _tracks;
};