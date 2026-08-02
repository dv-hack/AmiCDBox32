// CueParser.cpp
//
#include "CueParser.hpp"

#include "Sector.hpp"
#include "Track.hpp"
#include "FileSystem.hpp"

#include <sstream>
#include <charconv>
#include <algorithm>
#include <iostream>

// Riportato in vita dal blocco #if 0
static inline size_t msfToLBA(std::string_view msfStr)
{
    if (msfStr.size() < 8) return 0;

    int minutes = 0, seconds = 0, frames = 0;
    std::from_chars(msfStr.data(), msfStr.data() + 2, minutes);
    std::from_chars(msfStr.data() + 3, msfStr.data() + 5, seconds);
    std::from_chars(msfStr.data() + 6, msfStr.data() + 8, frames);

    return (minutes * 60 * 75) + (seconds * 75) + frames;
}

std::vector<Track> CueParser::parse(const std::string &cueFile)
{
    if (!FileSystem::exists(cueFile))
    {
        std::cout << "CUE file not found: " << cueFile << "\n";
        return {};
    }

    std::string cueContent;
    {
        FileSystem::File cueFileHandle = FileSystem::open(cueFile);
        // Assicurati che cueContent abbia la dimensione corretta prima di scriverci dentro!
        cueContent.resize(cueFileHandle.getSize());
        cueFileHandle.readAll(reinterpret_cast<uint8_t *>(cueContent.data()), cueFileHandle.getSize());
    }

    const FileSystem::FileInfo cueFileInfo = FileSystem::stat(cueFile);
    const std::string rootFolder = cueFile.substr(0, cueFile.size() - cueFileInfo.name.size());

    std::vector<Track> tracks;
    Track currentTrack;
    std::string currentBinFile = "";
    bool hasTrack = false;
    size_t absoluteLbaAccumulator = 0;

    // Catturiamo tutto per riferimento per operare su currentTrack
    auto handleTrackEntry = [&]()
    {
        if (!hasTrack) return;
        
        FileSystem::FileInfo fno = FileSystem::stat(rootFolder + currentBinFile);
        const size_t fileSectors = fno.size > 0 ? (fno.size / SectorSize::RAW) : 10000;

        // Calcoliamo la quantità di settori spettanti a questa traccia
        currentTrack._sectorCount = fileSectors - currentTrack._fileSectorOffset;
        
        // Prepariamo l'LBA di partenza per un'eventuale traccia successiva
        absoluteLbaAccumulator += currentTrack._sectorCount;
        
        tracks.push_back(std::move(currentTrack)); // Move, così spostiamo il file handle in modo sicuro
        hasTrack = false;
    };

    std::string line;
    std::istringstream stream{cueContent};

    while (std::getline(stream, line))
    {
        std::string_view svLine(line);
        size_t firstNonSpace = svLine.find_first_not_of(" \t\r\n");
        if (firstNonSpace == std::string_view::npos) continue;

        svLine = svLine.substr(firstNonSpace);
        size_t lastNonSpace = svLine.find_last_not_of(" \t\r\n");
        if (lastNonSpace != std::string_view::npos) svLine = svLine.substr(0, lastNonSpace + 1);

        if (svLine.starts_with("FILE"))
        {
            handleTrackEntry();

            size_t firstQuote = svLine.find('"');
            size_t lastQuote = svLine.rfind('"');
            if (firstQuote != std::string_view::npos && lastQuote != std::string_view::npos)
                currentBinFile = std::string(svLine.substr(firstQuote + 1, lastQuote - firstQuote - 1));
        }
        else if (svLine.starts_with("TRACK"))
        {
            currentTrack = Track();
            hasTrack = true;

            int trackNum = 0;
            std::from_chars(svLine.data() + 6, svLine.data() + 8, trackNum);
            currentTrack._number     = trackNum;
            currentTrack._file       = FileSystem::open(rootFolder + currentBinFile);
            currentTrack._type       = (svLine.ends_with("AUDIO")) ? Track::Type::Audio : Track::Type::Data;
            currentTrack._sectorSize = SectorSize::RAW;
            currentTrack._startLBA   = absoluteLbaAccumulator;
        }
        else if (svLine.starts_with("INDEX 01"))
        {
            size_t lastSpace = svLine.rfind(' ');
            if (lastSpace != std::string_view::npos)
            {
                std::string_view msf = svLine.substr(lastSpace + 1);
                currentTrack._fileSectorOffset = msfToLBA(msf);
            }
        }
    }

    handleTrackEntry(); // Non dimentichiamo di chiudere e pushare l'ultima traccia!

    return tracks;
}