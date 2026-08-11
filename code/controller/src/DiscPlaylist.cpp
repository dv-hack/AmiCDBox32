// DiscPlaylist.cpp
//
#include "DiscPlaylist.hpp"

#include <iostream>

// ------------------------------------------------------------------------------------------------
bool DiscPlaylist::scanDirectory( const FileSystem::Path &path )
{
    // if( !FileSystem::exists(path.string() ))
    // {
    //     std::cout << "[DiscPlaylist] Invalid directory to scan (" << path.string() << ")\n";
    //     return false;
    // }

    auto dir = FileSystem::openDir( path.string() );
    // if( !dir.isOpen() )
    // {
    //     std::cout << "[DiscPlaylist] Unable to open existing directory (" << path.string() << ")\n";
    //     return false;
    // }
    
    FileSystem::FileInfo entry;
    while( dir.readNext(entry) )
    {
        //std::cout << entry.name << (entry.isDirectory ? "/\n" : "\n");

        if( entry.isDirectory )
        {
            auto subDir = FileSystem::openDir( entry.name );            
            FileSystem::FileInfo subEntry;
            while( subDir.readNext(subEntry) )
            {
                //std::cout << "\t" <<subEntry.name << (subEntry.isDirectory ? "/\n" : "\n");

                if (subEntry.name.ends_with(".cue") || subEntry.name.ends_with(".CUE") || 
                    subEntry.name.ends_with(".iso") || subEntry.name.ends_with(".ISO")) 
                {
                    _playlist.push_back( FileSystem::makePath( subEntry.name) );
                    std::cout << "> " << _playlist.back().string() << std::endl;
                }
            }
        }
        else
        {
            // list any iso/cue in root folder
            if (entry.name.ends_with(".cue") || entry.name.ends_with(".CUE") || 
                entry.name.ends_with(".iso") || entry.name.ends_with(".ISO")) 
            {
                _playlist.push_back( FileSystem::makePath( entry.name) );
                std::cout << "> " << _playlist.back().string() << std::endl;
            }
        }
    }

    return !_playlist.empty();
}

// ------------------------------------------------------------------------------------------------
FileSystem::Path DiscPlaylist::currentEntry() const
{
    return _playlist.empty() 
        ? FileSystem::Path("")
        : _playlist[_currentIndex];
}

// ------------------------------------------------------------------------------------------------
FileSystem::Path DiscPlaylist::nextEntry()
{
    if (_playlist.empty()) 
        return FileSystem::Path("");
    
    _currentIndex = (_currentIndex + 1) % _playlist.size();
    return _playlist[_currentIndex];
}
