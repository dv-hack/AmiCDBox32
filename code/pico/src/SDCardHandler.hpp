// SDCardHandler.hpp
//
#pragma once

#include "ff.h"

#include <vector>
#include <string>

class SDCardHandler
{
public:
    bool mount();
    void unmount();

    bool fileExists( const std::string &filename ) const;

    std::vector<std::string> fileList( /*filters...*/);

private:
    FATFS _fileSystem;
    bool _mounted{false};
};