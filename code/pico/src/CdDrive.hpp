// CdDrive.hpp
//
#pragma once

#include <string>
#include <string_view>

enum class DriveState
{
    NoMedia,
    Idle,
    ReadingData,
    PlayingAudio
};

class CdDrive
{
public:
    CdDrive() = default;

    bool mount(std::string_view filename);
    void eject();

    void play(size_t targetLba);
    void playTrack(int trackNumber);
    void stop();

    void advanceSector();

    // Getters
    DriveState getState() const { return _state; }
    std::string_view getMountedFile() const { return _mountedFile; }
    size_t getCurrentLba() const { return _currentLba; }

private:
    DriveState _state = DriveState::NoMedia;
    std::string _mountedFile = "";
    size_t _currentLba = 0;
    size_t _totalSectors = 0;
    bool _isMultiTrack = false;

    // Internal mock helpers
    void streamAudioSector(size_t lba);
    void transferDataSector(size_t lba);
};