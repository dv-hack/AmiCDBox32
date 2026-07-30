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
    static std::string getVolumeName(const Disc& disc);
    static std::vector<IsoNode> getRootDirectory(const Disc& disc);

private:
    static uint32_t readLE32(const uint8_t* data) {
        return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    }
};