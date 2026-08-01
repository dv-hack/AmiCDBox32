// ISO9660Parser.hpp
//
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "Disc.hpp"

struct IsoNode
{
    std::string name;
    bool isDirectory;
    uint32_t lba;
    uint32_t size;
};

class Iso9660Parser
{
public:
    static std::string getVolumeName(Disc* disc);
    static std::vector<IsoNode> getRootDirectory(Disc* disc);

};