// DiscPlaylist.hpp
//
#pragma once

#include "FileSystem.hpp"

#include <vector>
#include <string>

class DiscPlaylist
{
public:
    bool scanDirectory( const FileSystem::Path &path );

    bool isEmpty() const { return _playlist.empty(); }

    FileSystem::Path currentEntry() const;
    FileSystem::Path nextEntry();

private:
    std::vector<FileSystem::Path> _playlist;
    size_t _currentIndex = 0;
};