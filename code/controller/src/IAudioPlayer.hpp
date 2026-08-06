// IAudioPlayer.hpp
//
#pragma once

#include <cstdint>

class Disc;

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;

    virtual void startPlayback( Disc *disc, uint32_t startLBA, uint32_t endLBA ) = 0;
    virtual void stopPlayback() = 0;
    virtual bool playFrame() = 0;

    virtual uint32_t currentLBA() const = 0;
    virtual bool isPlaying() const = 0;
};