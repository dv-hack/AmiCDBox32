// CueParser.hpp
//
#pragma once

#include <string>
#include <vector>

class Track;

class CueParser
{
public:
    static std::vector<Track> parse( const std::string &cueFile );
};