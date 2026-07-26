// CdDrive.hpp
//
#pragma once

#include "Disc.hpp"
#include "IAudioPlayer.hpp"

#include <string>
#include <vector>
#include <string_view>
#include <memory>

enum class DriveState {
    NoMedia,
    Idle,
    ReadingData,
    PlayingAudio
};

class CdDrive
{
public:
    CdDrive() = default;
    explicit CdDrive( IAudioPlayer *player ) 
    : _audioPlayer( player ) 
    {}


    bool mountDisc(std::string_view filename);
    void eject();
    
    bool readDataToBuffer( size_t lba, uint16_t blockCount, uint8_t* buffer );
    void play(size_t targetLba);
    void playTrack(int trackNumber);
    void stop();
    void advanceSector();

    DriveState       getState() const { return _state; }
    std::string_view getMountedFile() const { return _mountedFile; }
    size_t           getCurrentLba() const { return _currentLba; }

    Disc &disc() { return _disc;}

    void tickAudio();

private:
    void streamAudioSector(size_t lba);
    void transferDataSector(size_t lba);

private:
    DriveState _state = DriveState::NoMedia;
    //std::string _mountedFile = "";
    
    Disc _disc; // Table of Contents
    std::unique_ptr<IAudioPlayer> _audioPlayer;
    
    
    // size_t _currentLba = 0;
    // size_t _totalSectors = 0;
    // bool _isMultiTrack = false;
};