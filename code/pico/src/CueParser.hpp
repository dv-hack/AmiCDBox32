// CueParser.hpp
//
#pragma once

#include <string_view>

class Disc;

class CueParser
{
public:
    static Disc parse( std::string_view rootFolder, std::string_view cueContent);

private:
};