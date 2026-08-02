// CdDrive.hpp
//
#pragma once

#include "Disc.hpp"


#include <string>
#include <vector>
#include <cstdint>

class IAudioPlayer;

namespace SCSI
{
    constexpr uint8_t TEST_UNIT_READY = 0x00;
    constexpr uint8_t READ_CAPACITY   = 0x25;
    constexpr uint8_t READ_10         = 0x28;
    constexpr uint8_t READ_TOC        = 0x43;

    enum class Status : uint8_t
    {
        Good           = 0x00,
        CheckCondition = 0x02, // Error or no disc
        Busy           = 0x08
    };
}

class CdDrive
{
public:
    enum class State 
    {
        NoMedia,
        Ready,
        ReadingData,
        PlayingAudio
    };


public:
    CdDrive() = default;

    Disc *disc() { return _disc;}
    Disc *eject();
    void insertDisc( Disc *d );
    State state() const { return _state; }
    bool isReady() const { return _state == State::Ready; }

    void startAudioPlayback(int trackNumber, IAudioPlayer *player);
    void stopAudioPlayback();

    SCSI::Status scsi_executeCommandDescriptorBlock(const uint8_t* cdb, uint8_t* outBuffer, uint32_t& bytesTransferred);
        
private:
    void tickAudio();

private:
    State         _state{ State::NoMedia };
    Disc         *_disc;
    IAudioPlayer *_audioPlayer;
};