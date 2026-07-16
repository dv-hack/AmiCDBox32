// CdDrive.cpp
//
#include "CdDrive.hpp"

#include <iostream>

bool CdDrive::mount(std::string_view filename)
{
    _mountedFile = std::string(filename);
    _currentLba = 0;

    // Simple extension check to determine drive capabilities during debug
    if (filename.ends_with(".cue"))
    {
        _isMultiTrack = true;
        _totalSectors = 30000; // Mocked size
        std::cout << "[CdDrive] Loaded CUE sheet. Multitrack CD-DA enabled.\n";
    }
    else
    {
        _isMultiTrack = false;
        _totalSectors = 10000; // Mocked size
        std::cout << "[CdDrive] Loaded flat ISO. Data-only mode enabled.\n";
    }

    _state = DriveState::Idle;
    return true;
}

void CdDrive::eject()
{
    std::cout << "[CdDrive] Media ejected.\n";
    _mountedFile = "";
    _currentLba = 0;
    _totalSectors = 0;
    _isMultiTrack = false;
    _state = DriveState::NoMedia;
}

void CdDrive::play(size_t targetLba)
{
    if (_state == DriveState::NoMedia)
        return;

    _currentLba = targetLba;
    _state = DriveState::PlayingAudio;
    std::cout << "[CdDrive] Starting audio playback at LBA " << _currentLba << "\n";
}

void CdDrive::playTrack(int trackNumber)
{
    if (_state == DriveState::NoMedia)
        return;

    std::cout << "[CdDrive] Searching for track " << trackNumber << " (Mocked to LBA 10000)...\n";
    play(10000); // Mock jump to track
}

void CdDrive::stop()
{
    if (_state == DriveState::PlayingAudio)
    {
        _state = DriveState::Idle;
        std::cout << "[CdDrive] Audio playback stopped.\n";
    }
}

void CdDrive::advanceSector()
{
    if (_state == DriveState::NoMedia)
        return;

    if (_currentLba >= _totalSectors)
    {
        std::cout << "[CdDrive] End of disc reached.\n";
        _state = DriveState::Idle;
        return;
    }

    // Determine sector action based on simulated multitrack layout
    if (_isMultiTrack && _currentLba >= 10000)
    {
        _state = DriveState::PlayingAudio;
        streamAudioSector(_currentLba);
    }
    else
    {
        _state = DriveState::ReadingData;
        transferDataSector(_currentLba);
    }

    _currentLba++;
}

void CdDrive::streamAudioSector(size_t lba)
{
    // Hardware DAC/PWM streaming stub
    std::cout << "[Audio Engine] Outputting PCM LBA: " << lba << "\r" << std::flush;
}

void CdDrive::transferDataSector(size_t lba)
{
    // Parallel Port PIO transfer stub
    std::cout << "[PIO Engine] Delivering data sector: " << lba << " to parallel port.\n";
}