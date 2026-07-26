// CdDrive.cpp
//
#include "CdDrive.hpp"

#include "IAudioPlayer.hpp"
#include "LedDebugger.hpp"

#include "pico/stdlib.h"

#include <iostream>
#include <algorithm>
#include <memory.h>


// ------------------------------------------------------------------------------------------------
void CdDrive::insertDisc( Disc *d )
{
    _disc = d;
    _state = d == nullptr ? DriveState::NoMedia : DriveState::Ready;
}

// ------------------------------------------------------------------------------------------------
void CdDrive::eject()
{
    _disc = nullptr;
    _state = DriveState::NoMedia;
}
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void CdDrive::startAudioPlayback(int trackNumber, IAudioPlayer *player)
{
    if (_state == DriveState::NoMedia)
        return;

    if (_state == DriveState::NoMedia)
        return;

    // auto t = _disc->track( trackNumber );
    // if( !t || t->type == Track::Type::Audio )
    //     return;
    
    if(_state == DriveState::PlayingAudio)
        stopAudioPlayback();
    
    _state = DriveState::PlayingAudio;
    _audioPlayer = player;
    if( _audioPlayer )
        _audioPlayer->startPlayback( /*t*/ ); // @todo: passare track al player: il player esegue in autonomia il tick audio e chiede i dati a track
}
// ------------------------------------------------------------------------------------------------
void CdDrive::stopAudioPlayback()
{
    if (_state != DriveState::PlayingAudio)
        return;

    if( _audioPlayer )
        _audioPlayer->stopPlayback();

    _state = DriveState::Ready;
}
    
// ------------------------------------------------------------------------------------------------
bool CdDrive::readDataToBuffer( size_t lba, uint16_t count, uint8_t* buffer )
{
    if (_state == DriveState::NoMedia)
        return false;

    // @todo:
    // auto track = _map.getTrackAtSector( lba );
    // size_t relativeSector = (lba - track->startSector) + track->localOffsetSector;
    // relativeSector = lba;  // just for debug
    // // Leggiamo i settori sequenzialmente nel buffer
    // for (uint16_t i = 0; i < block_count; ++i)
    // {
    //     Sector s( track->sectorSize );
    //     auto ok = track->binFile->readSectorAt( relativeSector+i,s );

    //     if (ok != BinFile::ErrorCode::NoError)
    //     {
    //         //setErrorLed(true);
    //         return false;
    //     }

    //     //buffer.insert( buffer.end(), s.bytes.begin(), s.bytes.end() );
    //     memcpy(buffer + (i * 2048), &s.bytes[0], 2048);
    // }
    
    return true;
}