// Disc.cpp
//
#include "Disc.hpp"


// ------------------------------------------------------------------------------------------------
uint32_t Disc::totalSectors() const 
{
    if (_tracks.empty()) return 0;
    const auto& lastTrack = _tracks.back();
    return lastTrack.startLBA() + lastTrack.sectorCount();
}

// ------------------------------------------------------------------------------------------------
bool Disc::readSectors(uint32_t lba, uint16_t blockCount, uint8_t* buffer) const
{
    for (uint16_t i = 0; i < blockCount; ++i) {
        uint32_t currentLba = lba + i;
        Track* targetTrack = nullptr;

        // Trova la traccia che contiene questo LBA
        for ( auto& t : _tracks)
        {
            if (currentLba >= t.startLBA() && currentLba < (t.startLBA() + t.sectorCount()))
            {
                targetTrack = const_cast<Track*>(&t);
                break;
            }
        }

        if (!targetTrack) return false; // LBA fuori dal disco!

        // Calcola l'LBA relativo a quella specifica traccia
        uint32_t relLba = currentLba - targetTrack->startLBA();
        
        // Legge i 2048 byte nel punto giusto del buffer fornito da TinyUSB
        if (!targetTrack->readData(relLba, buffer + (i * 2048)))
        {
            return false; 
        }
    }
    return true;
}

// ------------------------------------------------------------------------------------------------
#if 0
size_t Disc::readTOC(uint8_t *buffer) const
{
    if (_tracks.empty())
        return 0; // Nessun disco/traccia presente
    uint8_t firstTrack = _tracks.front().number;
    uint8_t lastTrack = _tracks.back().number;

    // Calcolo della lunghezza dei dati:
    // 2 byte (First e Last track) + (N tracce * 8 byte) + (1 Lead-Out * 8 byte)
    uint16_t tocDataLength = 2 + (_tracks.size() + 1) * 8;

    // La dimensione totale dell'intero pacchetto è dataLength + 2 byte per il campo length stesso
    uint16_t totalResponseLen = tocDataLength + 2;

    if (maxLen < totalResponseLen)
    {
        // Se il buffer fornito dall'host è troppo piccolo, tronchiamo la risposta
        // per evitare buffer overflow.
        totalResponseLen = maxLen;
    }

    // --- HEADER (4 Byte) ---
    buffer[0] = (tocDataLength >> 8) & 0xFF; // MSB Lunghezza
    buffer[1] = tocDataLength & 0xFF;        // LSB Lunghezza
    buffer[2] = firstTrack;                  // Prima traccia
    buffer[3] = lastTrack;                   // Ultima traccia

    size_t offset = 4;

    // --- DESCRITTORI DELLE TRACCE (8 Byte ciascuno) ---
    for (const auto &t : _tracks)
    {
        if (offset + 8 > totalResponseLen)
            break;

        buffer[offset + 0] = 0x00; // Riservato

        // Control (4 bit) / ADR (4 bit)
        // 0x14 = Traccia Dati, 0x10 = Traccia Audio
        buffer[offset + 1] = (t.type == TrackType::Data) ? 0x14 : 0x10;

        buffer[offset + 2] = t.number; // Numero traccia
        buffer[offset + 3] = 0x00;     // Riservato

        // Start LBA della traccia (Big-Endian, 32 bit)
        buffer[offset + 4] = (t.startLba >> 24) & 0xFF;
        buffer[offset + 5] = (t.startLba >> 16) & 0xFF;
        buffer[offset + 6] = (t.startLba >> 8) & 0xFF;
        buffer[offset + 7] = t.startLba & 0xFF;

        offset += 8;
    }

    // --- DESCRITTORE LEAD-OUT (Traccia 0xAA) ---
    if (offset + 8 <= totalResponseLen)
    {
        uint32_t totalSectors = getTotalSectors();

        buffer[offset + 0] = 0x00; // Riservato

        // Il Lead-Out eredita tipicamente la natura dell'ultima traccia
        buffer[offset + 1] = (_tracks.back().type == TrackType::Data) ? 0x14 : 0x10;

        buffer[offset + 2] = 0xAA; // Magic number per la traccia di Lead-Out
        buffer[offset + 3] = 0x00; // Riservato

        // Start LBA del Lead-Out (coincide con il totale dei settori, Big-Endian)
        buffer[offset + 4] = (totalSectors >> 24) & 0xFF;
        buffer[offset + 5] = (totalSectors >> 16) & 0xFF;
        buffer[offset + 6] = (totalSectors >> 8) & 0xFF;
        buffer[offset + 7] = totalSectors & 0xFF;

        offset += 8;
    }

    return offset; // Ritorna i byte effettivamente scritti nel buffer
    return 0;
}
#endif