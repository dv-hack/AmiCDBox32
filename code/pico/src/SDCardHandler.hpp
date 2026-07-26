// SDCardHandler.hpp
//
#pragma once

#include <vector>
#include <string>
#include <memory>

class SDCardHandler
{
public:
    bool mount();
    void unmount();

    bool fileExists( const std::string &filename ) const;

    std::vector<std::string> fileList( /*filters...*/) const;

private:
    struct FileSystem;
    std::unique_ptr<FileSystem> _fileSystem;
};