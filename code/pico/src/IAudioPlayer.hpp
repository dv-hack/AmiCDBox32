// IAudioPlayer.hpp
//
#pragma once

//#include "Sector.hpp"

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;

    virtual void startPlayback() = 0;
    virtual void stopPlayback() = 0;
    
    //virtual void feedSector(const Sector &sector ) = 0;
};