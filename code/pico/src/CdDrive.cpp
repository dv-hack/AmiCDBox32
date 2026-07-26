// CdDrive.cpp
//
#include "CdDrive.hpp"

#include "LedDebugger.hpp"

#include "Sector.hpp"

#include "pico/stdlib.h"

#include <iostream>
#include <algorithm>
#include <memory.h>

// ------------------------------------------------------------------------------------------------
bool CdDrive::mount(std::string_view filename) 
{
    _map = buildCdMap( filename );

    if( !_map.isValid() )
    {
        std::cout << "[CdDrive] Unable to extract TOC for " << filename << "\n";
        return false;
    }     
    
    _mountedFile = std::string(filename);    
    _currentLba = 0;
    _isMultiTrack = (_map.tracks.size() > 1);
    _totalSectors = _map.tracks.empty() ? 0 : _map.tracks.back().endSector + 1;

    std::cout << "[CdDrive] Media mounted. Total layout sectors: " << _totalSectors 
              << " (MultiTrack: " << (_isMultiTrack ? "Yes" : "No") << ")\n";

    _state = DriveState::Idle;
    return true;
}

// ------------------------------------------------------------------------------------------------
void CdDrive::eject() 
{
    stop();

    _mountedFile = "";
    _currentLba = 0;
    _totalSectors = 0;
    _isMultiTrack = false;
    _state = DriveState::NoMedia;
    std::cout << "[CdDrive] Media logically ejected.\n";
}


// ------------------------------------------------------------------------------------------------
bool CdDrive::readDataToBuffer(size_t lba, uint16_t block_count, uint8_t* buffer)
{
    if (_state == DriveState::NoMedia)
        return false;

    auto track = _map.getTrackAtSector( lba );
    size_t relativeSector = (lba - track->startSector) + track->localOffsetSector;
    relativeSector = lba;  // just for debug
    // Leggiamo i settori sequenzialmente nel buffer
    for (uint16_t i = 0; i < block_count; ++i)
    {
        Sector s( track->sectorSize );
        auto ok = track->binFile->readSectorAt( relativeSector+i,s );

        if (ok != BinFile::ErrorCode::NoError)
        {
            //setErrorLed(true);
            return false;
        }

        // // Nota: Assumiamo che readSectorAt gestisca l'offset interno al file BIN
        // auto ok = _binFile.readSectorAt(lba + i, s); 
        
        // if (ok != BinFile::ErrorCode::NoError)
        // {
        //     setErrorLed(true);
        //     return false;
        // }

        // Copiamo i 2048 byte del settore dati nel buffer di destinazione
        //buffer.insert( buffer.end(), s.bytes.begin(), s.bytes.end() );
        memcpy(buffer + (i * 2048), &s.bytes[0], 2048);
    }
    
    return true;
}


// ------------------------------------------------------------------------------------------------
void CdDrive::play(size_t targetLba) 
{
    if (_state == DriveState::NoMedia)
        return;

    
        
    _currentLba = targetLba;

    const auto track = _map.getTrackAtSector(targetLba);
    if( track && track->type == TrackType::Audio )
    {
        // const auto filename = _map.rootFolder + track->binFilename;
   
        // if( _binFile.open( filename ) != BinFile::ErrorCode::NoError )
        // {
        //     std::cout << "[CdDrive] Could not play audio from file "<< filename <<"\n";
        //     return;
        // }    
        
        _state = DriveState::PlayingAudio;
        if( _audioPlayer )
            _audioPlayer->startPlayback();
    }    

    std::cout << "[CdDrive] Starting audio playback at LBA " << _currentLba << "\n";
}

// ------------------------------------------------------------------------------------------------
void CdDrive::playTrack(int trackNumber) 
{
    if (_state == DriveState::NoMedia)
        return;

    // find corresponding track
    auto found = std::find_if( _map.tracks.begin(), _map.tracks.end(), [n=trackNumber](const auto &t) 
    {
        return t.number == n;
    });

    // not found?
    if( found == std::end(_map.tracks) )
    {
        std::cout << "[CdDrive] Error: Track " << trackNumber << " not found in TOC.\n";
        return;
    }

    // not an audio track?
    if (found->type != TrackType::Audio) 
    {
        std::cout << "[CdDrive] Track " << trackNumber << " is a Data track (LBA " << found->startSector << "). Cannot play as audio.\n";
        return;
    }

    std::cout << "[CdDrive] Track " << trackNumber << " found. Starting audio at LBA " << found->startSector << ".\n";
    
    if(_state == DriveState::PlayingAudio)
        stop();
    
    play(found->startSector);
}

// ------------------------------------------------------------------------------------------------
void CdDrive::stop() 
{
    if (_state == DriveState::PlayingAudio) 
    {
        _state = DriveState::Idle;
        if (_audioPlayer)
            _audioPlayer->stopPlayback();
        
        //_binFile.close();
        std::cout << "[CdDrive] Audio playback stopped. File closed.\n";
    }
}

// ------------------------------------------------------------------------------------------------
void CdDrive::tickAudio() 
{
    if (_state != DriveState::PlayingAudio) 
        return;

    if (_currentLba >= _totalSectors) 
    {
        std::cout << "\n[CdDrive] End of audio tracks reached.\n";
        stop();
        return;
    }

    // Invia i 2352 byte correnti al lettore/buzzer
    streamAudioSector(_currentLba);

    // Incrementa l'LBA per il prossimo tick temporizzato
    _currentLba++;
}

// ------------------------------------------------------------------------------------------------
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

    // Se l'host fa uno Strobe mentre siamo in IDLE o DATA mode, serviamo un settore dati
    if (_state == DriveState::Idle || _state == DriveState::ReadingData) 
    {
        _state = DriveState::ReadingData;
        transferDataSector(_currentLba);
        _currentLba++;
    }
}

// ------------------------------------------------------------------------------------------------
void CdDrive::streamAudioSector(size_t lba) 
{
    // if( !_binFile.isOpen() )
    // {
    //     std::cout << "[CdDrive] Attempt to stream audio from unopened BIN file.\n";
    //     return;
    // }

    // 1. Recupera la traccia corrente per calcolare l'offset relativo al file binario
    const auto track = _map.getTrackAtSector(lba);
    if (!track) {
        std::cout << "[CdDrive] Error: No track mapping found for LBA " << lba << "\n";
        return;
    }

    // 2. Calcola il settore relativo interno a questo specifico file .bin
    size_t relativeSector = (lba - track->startSector) + track->localOffsetSector;

    Sector s(2352);
    //auto ok = _binFile.readSectorAt( relativeSector, s);
    auto ok = track->binFile->readSectorAt( relativeSector, s);

    if( ok == BinFile::ErrorCode::EndOfFile )
    {
        std::cout << "[CdDrive] End of track reached. Stopping audio playback.\n";
        stop();
        return;
    }

    if( ok != BinFile::ErrorCode::NoError )
    {
        std::cout << "[CdDrive] Error during reading audio from BIN file.\n";
        return;        
    }

    if( _audioPlayer )
        _audioPlayer->feedSector( s );
}

// ------------------------------------------------------------------------------------------------
void CdDrive::transferDataSector(size_t lba) 
{
    std::cout << "[PIO Engine] Delivering data sector: " << lba << " to parallel port.\n";
}