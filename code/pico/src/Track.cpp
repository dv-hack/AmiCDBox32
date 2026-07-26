#pragma once
#include <cstdint>
// Include per la tua classe di gestione file (es. FatFs)

enum class TrackType { Data, Audio };

class Track {
public:
    uint8_t number;
    uint32_t startLba;
    uint32_t sectorCount;
    uint16_t sectorSize; // 2048 (ISO), 2352 (BIN/RAW)
    TrackType type;
    
    // Il puntatore o l'oggetto file aperto su FatFS
    // FileHandle* file; 

    Track(uint8_t num, uint32_t start, uint32_t count, uint16_t size, TrackType t)
        : number(num), startLba(start), sectorCount(count), sectorSize(size), type(t) {}

    // Legge il payload di 2048 byte per USB MSC (estraendolo se il settore è 2352)
    bool readPayload(uint32_t relLba, uint8_t* outBuffer) const {
        // 1. Calcola l'offset nel file: relLba * sectorSize
        // 2. Esegui la seek(offset)
        // 3. Se sectorSize == 2048 -> f_read direttamente in outBuffer
        // 4. Se sectorSize == 2352 (Dati) -> f_read in un buffer temporaneo, copia i byte da [16] a [2063] in outBuffer
        return true; 
    }
};