// Mounter.cpp
//
#include "Mounter.hpp"

#include "CueParser.hpp"
#include "Disc.hpp"
#include "Track.hpp"

#include "FileSystem.hpp"

#include <iostream>



// ------------------------------------------------------------------------------------------------
Disc* Mounter::mount( const std::string &filename )
{
    if( !FileSystem::exists( filename ))
    {
        std::cout << "File not found\n";
        return nullptr;
    }   


    if( filename.ends_with(".iso") || filename.ends_with(".ISO") )
    {
        Track dataTrack = Track::makeISOTrack( filename );    

        Disc *d = new Disc;
        d->_tracks.push_back( std::move(dataTrack) );

        std::cout << "Disc " << filename << " mounted\n";
        return d;
    }

    if( filename.ends_with(".cue") || filename.ends_with(".CUE") )
    {
        Disc *d = new Disc;
        d->_tracks = CueParser::parse( filename );
        
        if (d->_tracks.empty())
        {
            delete d;
            return nullptr;
        }
        
        std::cout << "Disc " << filename << " mounted with " << d->_tracks.size() << " tracks\n";
        return d;
    }


    return nullptr;

}