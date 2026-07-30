// CdDrive.cpp
//
#include "CdDrive.hpp"

#include "Endianess.hpp"
#include "IAudioPlayer.hpp"


#include <iostream>
#include <cstring>




// ------------------------------------------------------------------------------------------------
void CdDrive::insertDisc( Disc d )
{
    _disc = std::move( d );
    _state = _disc.isEmpty() ? State::NoMedia : State::Ready;
}

// ------------------------------------------------------------------------------------------------
void CdDrive::eject()
{
    _disc = {};
    _state = State::NoMedia;
}
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void CdDrive::startAudioPlayback(int trackNumber, IAudioPlayer *player)
{
    if (_state == State::NoMedia)
        return;

    // auto t = _disc->track( trackNumber );
    // if( !t || t->type == Track::Type::Audio )
    //     return;
    
    if(_state == State::PlayingAudio)
        stopAudioPlayback();
    
    _state = State::PlayingAudio;
    _audioPlayer = player;
    if( _audioPlayer )
        _audioPlayer->startPlayback( /*t*/ ); // @todo: passare track al player: il player esegue in autonomia il tick audio e chiede i dati a track
}
// ------------------------------------------------------------------------------------------------
void CdDrive::stopAudioPlayback()
{
    if (_state != State::PlayingAudio)
        return;

    if( _audioPlayer )
        _audioPlayer->stopPlayback();

    _state = State::Ready;
}

// ------------------------------------------------------------------------------------------------
SCSI::Status CdDrive::scsi_executeCommandDescriptorBlock(const uint8_t* cdb, uint8_t* outBuffer, uint32_t& bytesTransferred)
{
    bytesTransferred = 0;

    if (!isReady()) {
        return SCSI::Status::CheckCondition; // Media Not Present
    }

    const uint8_t opcode = cdb[0];

    switch (opcode) 
    {
        case SCSI::TEST_UNIT_READY: {
            return SCSI::Status::Good;
        }

        case SCSI::READ_CAPACITY: {
            uint32_t totalSectors = _disc.totalSectors();
            uint32_t lastLba = (totalSectors > 0) ? (totalSectors - 1) : 0;

            // Scrittura pulita dei dati in Big-Endian per il bus SCSI
            Endianess::writeBE32(&outBuffer[0], lastLba);
            Endianess::writeBE32(&outBuffer[4], 2048); // Block size standard CD

            bytesTransferred = 8;
            return SCSI::Status::Good;
        }

        case SCSI::READ_10: {
            // Estrazione LBA e conteggio settori tramite utility
            uint32_t lba = Endianess::readBE32(&cdb[2]);
            uint16_t sectorCount = Endianess::readBE16(&cdb[7]);

            if (sectorCount == 0) return SCSI::Status::Good;

            if (_disc.readSectors(lba, sectorCount, outBuffer)) {
                bytesTransferred = sectorCount * 2048;
                return SCSI::Status::Good;
            }
            return SCSI::Status::CheckCondition;
        }

        case SCSI::READ_TOC:
        {
            bool msf = (cdb[1] & 0x02) != 0; // Bit 1 = Formato MSF richiesto dall'host
            uint8_t startTrackReq = cdb[6];   // Traccia da cui iniziare a elencare
            uint16_t allocLen = Endianess::readBE16(&cdb[7]);

            const size_t trackCount = _disc.trackCount();
            const uint8_t firstTrack = 1;
            const uint8_t lastTrack = static_cast<uint8_t>(trackCount);

            if (startTrackReq > lastTrack && startTrackReq != 0xAA)
                startTrackReq = firstTrack;

            uint8_t tocBuffer[256] = {0};
            uint32_t offset = 4; // I primi 4 byte sono l'Header TOC SCSI

            // Helper lambda per convertire LBA in MSF (75 Frame = 1 secondo)
            auto lbaToMsf = [](uint32_t lba, uint8_t& m, uint8_t& s, uint8_t& f) 
            {
                lba += 150; // Offset standard SCSI di 2 secondi (150 frame) per MSF
                m = lba / (60 * 75);
                s = (lba / 75) % 60;
                f = lba % 75;
            };

            auto trackControlFlag = []( const Track & t)
            {
                return t.type() == Track::Type::Data ? 0x14 : 0x00;   // 0x14 = Data, 0x00 = Audio
            };

            // 1. Aggiungiamo i descrittori per ciascuna traccia reale
            for (size_t i = 0; i < trackCount; ++i)
            {
                const Track& track = _disc.track(i);

                if (track.number() < startTrackReq && startTrackReq != 0xAA)
                    continue;

                tocBuffer[offset + 0] = 0;                    // Reserved
                tocBuffer[offset + 1] = trackControlFlag(track);
                tocBuffer[offset + 2] = track.number();         // Numero traccia reale
                tocBuffer[offset + 3] = 0;                    // Reserved

                if (msf) 
                {
                    uint8_t m, s, f;
                    lbaToMsf(track.startLBA(), m, s, f);
                    tocBuffer[offset + 4] = 0;
                    tocBuffer[offset + 5] = m;
                    tocBuffer[offset + 6] = s;
                    tocBuffer[offset + 7] = f;
                }
                else
                    Endianess::writeBE32(&tocBuffer[offset + 4], track.startLBA());
                offset += 8;
            }

            // 2. Aggiungiamo il descrittore obbligatorio di Lead-Out (Traccia 0xAA)
            const Track& lastTrackObj = _disc.track(trackCount - 1);
            tocBuffer[offset + 0] = 0;
            tocBuffer[offset + 1] = trackControlFlag(lastTrackObj);
            tocBuffer[offset + 2] = 0xAA; // 0xAA identifica la Lead-Out in SCSI
            tocBuffer[offset + 3] = 0;

            uint32_t leadOutLba = _disc.totalSectors();
            if (msf)
            {
                uint8_t m, s, f;
                lbaToMsf(leadOutLba, m, s, f);
                tocBuffer[offset + 4] = 0;
                tocBuffer[offset + 5] = m;
                tocBuffer[offset + 6] = s;
                tocBuffer[offset + 7] = f;
            } 
            else
                Endianess::writeBE32(&tocBuffer[offset + 4], leadOutLba);
            offset += 8;

            // 3. Intestazione della TOC
            uint16_t dataLength = static_cast<uint16_t>(offset - 2);
            Endianess::writeBE16(&tocBuffer[0], dataLength); // Data length esclusi i primi 2 byte
            tocBuffer[2] = firstTrack;
            tocBuffer[3] = lastTrack;

            // Copiamo nel buffer dell'host rispettando l'Allocation Length
            uint32_t toCopy = (allocLen < offset) ? allocLen : offset;
            std::memcpy(outBuffer, tocBuffer, toCopy);
            bytesTransferred = toCopy;
            return SCSI::Status::Good;
        }

        default:
            return SCSI::Status::CheckCondition;
    }
}