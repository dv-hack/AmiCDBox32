// CdDrive.hpp
//
#pragma once

#include "CdMap.hpp"
#include "IAudioPlayer.hpp"
#include "BinFile.hpp"

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


    bool mount(std::string_view filename);
    void eject();
    
    void play(size_t targetLba);
    void playTrack(int trackNumber);
    void stop();
    void advanceSector();

    DriveState       getState() const { return _state; }
    std::string_view getMountedFile() const { return _mountedFile; }
    size_t           getCurrentLba() const { return _currentLba; }

    void tickAudio();

private:
    DriveState _state = DriveState::NoMedia;
    std::string _mountedFile = "";
    
    CdMap _map; // Table of Contents
    std::unique_ptr<IAudioPlayer> _audioPlayer;
    BinFile _binFile;
    
    
    size_t _currentLba = 0;
    size_t _totalSectors = 0;
    bool _isMultiTrack = false;

    void streamAudioSector(size_t lba);
    void transferDataSector(size_t lba);
};