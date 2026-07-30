// Mounter.cpp
//
#include "Mounter.hpp"

#include "Disc.hpp"
#include "Track.hpp"

#include "FileSystem.hpp"

#include <iostream>



// ------------------------------------------------------------------------------------------------
Disc Mounter::mount( const std::string &filename )
{
    if( filename.ends_with(".iso") || filename.ends_with(".ISO") )
    {
        if( !FileSystem::exists( filename ))
        {
            std::cout << "File not found\n";
            return {};
        }   

        Track dataTrack = Track::makeISOTrack( filename );    

        Disc d;
        d._tracks.push_back( std::move(dataTrack) );

        std::cout << "Disc " << filename << " mounted\n";
        return d;
    }

    if( filename.ends_with(".cue") || filename.ends_with(".CUE") )
    {}  


    return {};

}