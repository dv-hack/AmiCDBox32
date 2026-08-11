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
    constexpr uint8_t TEST_UNIT_READY  = 0x00;
    constexpr uint8_t START_STOP_UNIT  = 0x1B;
    constexpr uint8_t READ_CAPACITY    = 0x25;
    constexpr uint8_t READ_10          = 0x28;
    constexpr uint8_t READ_SUB_CHANNEL = 0x42;
    constexpr uint8_t READ_TOC         = 0x43;
    //constexpr uint8_t PLAY_AUDIO_TRACK= 0x48; // unused?
    constexpr uint8_t PLAY_AUDIO_10    = 0x45;
    constexpr uint8_t PLAY_AUDIO_MSF   = 0x47;
    constexpr uint8_t STOP_PLAY_SCAN   = 0x4E;

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

    void setAudioPlayer(IAudioPlayer *player) { _audioPlayer = player; }

    Disc *disc() { return _disc;}
    Disc *eject();
    void insertDisc( Disc *d );
    State state() const { return _state; }
    bool isReady() const { return _state == State::Ready; }
    void updateAudioPlayback();

    SCSI::Status scsi_executeCommandDescriptorBlock(const uint8_t* cdb, uint8_t* outBuffer, uint16_t bufSize, uint32_t& bytesTransferred);
        
private:
    void startAudioPlayback( uint32_t startLBA, uint32_t endLBA );
    void stopAudioPlayback();

private:
    State         _state{ State::NoMedia };
    Disc         *_disc { nullptr };
    IAudioPlayer *_audioPlayer;
};