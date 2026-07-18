#include "CdDrive.hpp"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <iostream>
#include <algorithm>

// Assign the hardware SPI instance to a runtime constant to bypass the SDK's C-style macro restriction
spi_inst_t* const spiHardwareInstance = spi0;

constexpr uint mosiPin = 19;
constexpr uint misoPin = 16;
constexpr uint sckPin = 18;
constexpr uint csPin = 17;

bool CdDrive::mount(std::string_view filename) {
   _mountedFile = std::string(filename);
    _currentLba = 0;

    if (filename.ends_with(".cue")) {
        _isMultiTrack = true;
        _totalSectors = 30000;
        std::cout << "[CdDrive] Loaded CUE sheet template. Multitrack CD-DA ready.\n";
    } else {
        _isMultiTrack = false;
        _totalSectors = 10000;
        std::cout << "[CdDrive] Loaded flat ISO template. Data-only mode ready.\n";
    }

    _state = DriveState::Idle;
    return true;
}

void CdDrive::eject() {
    _mountedFile = "";
    _currentLba = 0;
    _totalSectors = 0;
    _isMultiTrack = false;
    _state = DriveState::NoMedia;
    std::cout << "[CdDrive] Media logically ejected.\n";
}

void CdDrive::play(size_t targetLba) {
    if (_state == DriveState::NoMedia) return;
    
    _currentLba = targetLba;
    _state = DriveState::PlayingAudio;
    std::cout << "[CdDrive] Starting audio playback at LBA " << _currentLba << "\n";
}

void CdDrive::playTrack(int trackNumber) {
    if (_state == DriveState::NoMedia) return;

    std::cout << "[CdDrive] Searching map for track " << trackNumber << " (Mocked to LBA 10000)...\n";
    play(10000);
}

void CdDrive::stop() {
    if (_state == DriveState::PlayingAudio) {
        _state = DriveState::Idle;
        std::cout << "[CdDrive] Audio playback stopped.\n";
    }
}

void CdDrive::advanceSector() {
    if (_state == DriveState::NoMedia) return;

    if (_currentLba >= _totalSectors) {
        std::cout << "[CdDrive] End of disc reached.\n";
        _state = DriveState::Idle;
        return;
    }

    if (_isMultiTrack && _currentLba >= 10000) {
        _state = DriveState::PlayingAudio;
        streamAudioSector(_currentLba);
    } else {
        _state = DriveState::ReadingData;
        transferDataSector(_currentLba);
    }

    _currentLba++;
}

void CdDrive::streamAudioSector(size_t lba) {
    std::cout << "[Audio Engine] Outputting PCM LBA: " << lba << "\r" << std::flush;
}

void CdDrive::transferDataSector(size_t lba) {
    std::cout << "[PIO Engine] Delivering data sector: " << lba << " to parallel port.\n";
}