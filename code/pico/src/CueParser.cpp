// CueParser.cpp
//
#include "CueParser.hpp"

#include <sstream>
#include <iostream>
#include <charconv>
#include <algorithm>

// ------------------------------------------------------------------------------------------------
// converts minutes, seconds and frames to LBA
static inline size_t msfToLBA(std::string_view msfStr)
{
    if (msfStr.size() < 8)
        return 0;

    int minutes = 0, seconds = 0, frames = 0;

    std::from_chars(msfStr.data(), msfStr.data() + 2, minutes);
    std::from_chars(msfStr.data() + 3, msfStr.data() + 5, seconds);
    std::from_chars(msfStr.data() + 6, msfStr.data() + 8, frames);

    return (minutes * 60 * 75) + (seconds * 75) + frames;
}

// ------------------------------------------------------------------------------------------------
CdMap CueParser::parse(std::string_view cueContent)
{
    CdMap map;
    std::string line;
    std::istringstream stream{std::string(cueContent)};

    Track currentTrack;
    bool hasTrack = false;

    while (std::getline(stream, line))
    {
        std::string_view svLine(line);

        size_t firstNonSpace = svLine.find_first_not_of(" \t\r\n");
        if (firstNonSpace == std::string_view::npos)
            continue;
        svLine = svLine.substr(firstNonSpace);

        if (svLine.starts_with("FILE"))
        {
            size_t firstQuote = svLine.find('"');
            size_t lastQuote = svLine.rfind('"');
            if (firstQuote != std::string_view::npos && lastQuote != std::string_view::npos)
            {
                map.binFilename = std::string(svLine.substr(firstQuote + 1, lastQuote - firstQuote - 1));
            }
        }
        else if (svLine.starts_with("TRACK"))
        {
            if (hasTrack)
            {
                map.tracks.push_back(currentTrack);
            }

            currentTrack = Track();
            hasTrack = true;

            int trackNum = 0;
            std::from_chars(svLine.data() + 6, svLine.data() + 8, trackNum);
            currentTrack.number = trackNum;

            if (svLine.ends_with("AUDIO"))
            {
                currentTrack.type = TrackType::Audio;
            }
            else
            {
                currentTrack.type = TrackType::Data;
            }
        }
        else if (svLine.starts_with("INDEX 01"))
        {
            size_t lastSpace = svLine.rfind(' ');
            if (lastSpace != std::string_view::npos)
            {
                std::string_view msf = svLine.substr(lastSpace + 1);
                currentTrack.startSector = msfToLBA(msf);

                if (!map.tracks.empty())
                {
                    map.tracks.back().endSector = currentTrack.startSector - 1;
                }
            }
        }
    }

    if (hasTrack)
    {
        map.tracks.push_back(currentTrack);
    }

    return map;
}