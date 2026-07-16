// CueParser.hpp
//
#pragma once

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
    size_t startSector = 0; // Logical Block Address (LBA) in the .bin file
    size_t endSector = 0;   // End LBA (0 if it is the last track)
};

struct CdMap
{
    std::string binFilename;
    std::vector<Track> tracks;

    const Track *getTrackAtSector(size_t sector) const
    {
        for (const auto &track : tracks)
        {
            if (sector >= track.startSector && (track.endSector == 0 || sector <= track.endSector))
            {
                return &track;
            }
        }
        return nullptr;
    }
};

class CueParser
{
public:
    static CdMap parse(std::string_view cueContent);

private:
};