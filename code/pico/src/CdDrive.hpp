// CdDrive.hpp
//
#pragma once


#include <string>
#include <vector>
#include <string_view>
#include <memory>

class Disc;
class IAudioPlayer;

enum class DriveState {
    NoMedia,
    Ready,
    ReadingData,
    PlayingAudio
};

class CdDrive
{
public:
    CdDrive() = default;

    Disc *disc() { return _disc;}
    void eject();
    void insertDisc( Disc *d );
    DriveState state() const { return _state; }
    
    void startAudioPlayback(int trackNumber, IAudioPlayer *player);
    void stopAudioPlayback();
    
    bool readDataToBuffer( size_t lba, uint16_t count, uint8_t* buffer );
    
private:
    void tickAudio();

private:
    DriveState _state{ DriveState::NoMedia };
    Disc      *_disc { nullptr };
    IAudioPlayer *_audioPlayer;
};