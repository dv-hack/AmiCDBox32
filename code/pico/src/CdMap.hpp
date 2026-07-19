// CdMap.hpp
//
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
    std::string binFilename = "";     // <-- OGNI TRACCIA HA IL SUO FILE .BIN
    size_t startSector = 0;           // LBA Assoluto sul CD virtuale (per la stringa di boot Amiga)
    size_t endSector = 0;             // LBA Assoluto di fine traccia
    size_t localOffsetSector = 0;     // Offset interno al file specifico (spesso 150 per il pregap)
};

struct CdMap
{
    std::string rootFolder;
    std::vector<Track> tracks;

    bool isValid() const { return !rootFolder.empty() && !tracks.empty(); }

    const Track *getTrackAtSector(size_t sector) const
    {
        for (const auto &track : tracks)
            if (sector >= track.startSector && (track.endSector == 0 || sector <= track.endSector))
                return &track;
        return nullptr;
    }
};

CdMap buildCdMap( std::string_view filename );