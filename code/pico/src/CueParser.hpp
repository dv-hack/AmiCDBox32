// CueParser.hpp
//
#pragma once

#include <string_view>

class CdMap;

class CueParser
{
public:
    static CdMap parse( std::string_view rootFolder, std::string_view cueContent);

private:
};