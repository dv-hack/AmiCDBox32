// CdDrive.cpp
//
#include "CdDrive.hpp"

#include "Endianess.hpp"
#include "IAudioPlayer.hpp"


#include <iostream>
#include <cstring>

#pragma region helpers
static inline void lbaToMsf(uint32_t lba, uint8_t &m, uint8_t &s, uint8_t &f)
{
    lba += 150; // Lead-in offset CD
    m = lba / (60 * 75);
    s = (lba / 75) % 60;
    f = lba % 75;
}

static inline uint32_t msfToLba(uint8_t m, uint8_t s, uint8_t f)
{
    uint32_t frames = (m * 60 * 75) + (s * 75) + f;
    return (frames >= 150) ? (frames - 150) : 0;
}
#pragma endregion

// ------------------------------------------------------------------------------------------------
void CdDrive::insertDisc( Disc *d )
{
    _disc = d;
    _state = (!_disc || _disc->isEmpty()) ? State::NoMedia : State::Ready;
}

// ------------------------------------------------------------------------------------------------
Disc *CdDrive::eject()
{
    Disc *disc = _disc;
    _disc = nullptr;
    _state = State::NoMedia;
    return disc;
}
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void CdDrive::startAudioPlayback(uint32_t startLBA, uint32_t endLBA)
{
    if (_state == State::NoMedia)
        return;

    if (_state == State::PlayingAudio)
        stopAudioPlayback();

    _state = State::PlayingAudio;
    if (_audioPlayer)
        _audioPlayer->startPlayback(_disc, startLBA, endLBA);
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
void CdDrive::updateAudioPlayback()
{
    if (_state != State::PlayingAudio || !_audioPlayer)
        return;

    if (!_audioPlayer->playFrame())
        stopAudioPlayback(); // error or end of track
}

// ------------------------------------------------------------------------------------------------
SCSI::Status CdDrive::scsi_executeCommandDescriptorBlock(const uint8_t *cdb, uint8_t *outBuffer, uint32_t &bytesTransferred)
{
    bytesTransferred = 0;

    if (!isReady())
        return SCSI::Status::CheckCondition; // Media Not Present

    const uint8_t opcode = cdb[0];

    switch (opcode)
    {
    case SCSI::TEST_UNIT_READY:
        return SCSI::Status::Good;

    case SCSI::READ_CAPACITY:
    {
        uint32_t totalSectors = _disc->totalSectors();
        uint32_t lastLba = (totalSectors > 0) ? (totalSectors - 1) : 0;

        // Scrittura pulita dei dati in Big-Endian per il bus SCSI
        Endianess::writeBE32(&outBuffer[0], lastLba);
        Endianess::writeBE32(&outBuffer[4], SectorSize::ISO ); // Block size standard CD

        bytesTransferred = 8;
        return SCSI::Status::Good;
    }

    case SCSI::READ_10:
    {
        // Estrazione LBA e conteggio settori tramite utility
        uint32_t lba = Endianess::readBE32(&cdb[2]);
        uint16_t sectorCount = Endianess::readBE16(&cdb[7]);

        if (sectorCount == 0)
            return SCSI::Status::Good;

        if (_disc->readSectors(lba, sectorCount, SectorSize::ISO, outBuffer))
        {
            bytesTransferred = sectorCount * SectorSize::ISO;
            return SCSI::Status::Good;
        }
        return SCSI::Status::CheckCondition;
    }

    case SCSI::READ_TOC:
    {
        bool msf = (cdb[1] & 0x02) != 0; // Se true, l'host vuole gli indirizzi in MSF
        uint8_t startTrack = cdb[6];
        uint16_t allocLen = Endianess::readBE16(&cdb[7]);

        if (!_disc || _disc->trackCount() == 0)
            return SCSI::Status::CheckCondition;

        uint8_t numTracks = static_cast<uint8_t>(_disc->trackCount());
        if (startTrack == 0)
            startTrack = 1;

        uint8_t response[400] = {0};
        uint32_t ptr = 4;

        // 1. Tracce audio e dati
        for (uint8_t i = startTrack; i <= numTracks; ++i)
        {
            const auto &t = _disc->track(i - 1);
            response[ptr++] = 0x00;                                           // Reserved
            response[ptr++] = (t.type() == Track::Type::Audio) ? 0x00 : 0x14; // Control/ADR
            response[ptr++] = t.number();                                     // Numero traccia
            response[ptr++] = 0x00;                                           // Reserved

            if (msf)
            {
                uint8_t m, s, f;
                lbaToMsf(t.startLBA(), m, s, f);
                response[ptr++] = 0x00;
                response[ptr++] = m;
                response[ptr++] = s;
                response[ptr++] = f;
            }
            else
            {
                Endianess::writeBE32(&response[ptr], t.startLBA());
                ptr += 4;
            }
        }

        // 2. FONDAMENTALE PER WINUAE: Traccia di Lead-Out (0xAA)
        uint32_t leadOutLba = _disc->totalSectors();
        response[ptr++] = 0x00; // Reserved
        response[ptr++] = 0x00; // Control/ADR
        response[ptr++] = 0xAA; // Track 0xAA = Lead-Out
        response[ptr++] = 0x00; // Reserved

        if (msf)
        {
            uint8_t m, s, f;
            lbaToMsf(leadOutLba, m, s, f);
            response[ptr++] = 0x00;
            response[ptr++] = m;
            response[ptr++] = s;
            response[ptr++] = f;
        }
        else
        {
            Endianess::writeBE32(&response[ptr], leadOutLba);
            ptr += 4;
        }

        // Header della TOC
        uint16_t dataLength = static_cast<uint16_t>(ptr - 2);
        Endianess::writeBE16(&response[0], dataLength);
        response[2] = 1;         // Prima traccia
        response[3] = numTracks; // Ultima traccia

        uint32_t toCopy = (allocLen < ptr) ? allocLen : ptr;
        std::memcpy(outBuffer, response, toCopy);
        bytesTransferred = toCopy;
        return SCSI::Status::Good;
    }

    case SCSI::START_STOP_UNIT:
    {
        bool loEj = (cdb[4] & 0x02) != 0;
        bool start = (cdb[4] & 0x01) != 0;

        if (loEj && !start)
        {
            stopAudioPlayback();
            eject(); // Sgancia il disco
        }
        else if (!start)
        {
            stopAudioPlayback(); // Ferma solo il motore/riproduzione
        }
        return SCSI::Status::Good;
    }

    case SCSI::PLAY_AUDIO_10:
    {
        // Estrazione indirizzi
        const uint32_t startLba = Endianess::readBE32(&cdb[2]);
        const uint16_t blockCount = Endianess::readBE16(&cdb[7]);

        startAudioPlayback(startLba, startLba + blockCount);
        return SCSI::Status::Good;
    }

    case SCSI::STOP_PLAY_SCAN:
    {
        stopAudioPlayback();
        return SCSI::Status::Good;
    }

    case SCSI::READ_SUB_CHANNEL:
    {
        bool msf = (cdb[1] & 0x02) != 0;
        uint8_t subQFormat = cdb[2];
        uint16_t allocLen = Endianess::readBE16(&cdb[6]);

        uint8_t response[16] = {0};

        // Header Sub-Channel
        response[0] = 0x00;                                          // Reserved
        response[1] = (_state == State::PlayingAudio) ? 0x11 : 0x13; // 0x11 = Playing, 0x13 = Stopped
        response[2] = 0x00;                                          // Sub-channel Data Length MSB
        response[3] = 0x0C;                                          // Sub-channel Data Length LSB (12 byte di payload)

        // Se l'host richiede le informazioni di posizione Sub-Q (Format 0x01)
        if (subQFormat == 0x01 || subQFormat == 0x00)
        {
            response[4] = 0x01; // Data Format Code
            response[5] = 0x00; // Control/ADR (0x00 per audio)

            uint32_t currentLBA = _audioPlayer ? _audioPlayer->currentLBA() : 0;

            // Trova la traccia in riproduzione corrente
            uint8_t currentTrackNum = 1;
            uint32_t relLba = 0;

            for (size_t i = 0; i < _disc->trackCount(); ++i)
            {
                const auto &t = _disc->track(i);
                if (currentLBA >= t.startLBA() && currentLBA < (t.startLBA() + t.sectorCount()))
                {
                    currentTrackNum = t.number();
                    relLba = currentLBA - t.startLBA();
                    break;
                }
            }

            response[6] = currentTrackNum; // Traccia corrente
            response[7] = 0x01;            // Index 01

            auto lbaToMsf = [](uint32_t lba, uint8_t &m, uint8_t &s, uint8_t &f)
            {
                lba += 150;
                m = lba / (60 * 75);
                s = (lba / 75) % 60;
                f = lba % 75;
            };

            uint8_t absM, absS, absF;
            uint8_t relM, relS, relF;
            lbaToMsf(currentLBA, absM, absS, absF);
            lbaToMsf(relLba, relM, relS, relF);

            // Posizione Assoluta (Byte 8-11) e Relativa (Byte 12-15)
            response[8] = 0;
            response[9] = absM;
            response[10] = absS;
            response[11] = absF;
            response[12] = 0;
            response[13] = relM;
            response[14] = relS;
            response[15] = relF;
        }

        uint32_t toCopy = (allocLen < sizeof(response)) ? allocLen : sizeof(response);
        std::memcpy(outBuffer, response, toCopy);
        bytesTransferred = toCopy;
        return SCSI::Status::Good;
    }

    case SCSI::PLAY_AUDIO_MSF:
    {
        uint8_t startM = cdb[3];
        uint8_t startS = cdb[4];
        uint8_t startF = cdb[5];

        uint8_t endM = cdb[6];
        uint8_t endS = cdb[7];
        uint8_t endF = cdb[8];

        uint32_t startLba = msfToLba(startM, startS, startF);
        uint32_t endLba   = msfToLba(endM, endS, endF);

        if (endLba > startLba && _audioPlayer)
        {
            _audioPlayer->startPlayback(_disc, startLba, endLba);
            _state = State::PlayingAudio;
        }

        return SCSI::Status::Good;
    }    

    default:
        return SCSI::Status::CheckCondition;
    }
}