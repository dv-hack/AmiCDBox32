// CueParser.cpp
//
#include "CueParser.hpp"

#include "ff.h"

#include "CdMap.hpp"

#include <sstream>
#include <charconv>
#include <algorithm>

// ------------------------------------------------------------------------------------------------
// converts minutes, seconds and frames to LBA
static inline size_t msfToLBA(std::string_view msfStr)
{
    if (msfStr.size() < 8)
        return 0;

    int minutes = 0, seconds = 0, frames = 0;

    std::from_chars(msfStr.data(), msfStr.data() + 2, minutes);
    std::from_chars(msfStr.data() + 3, msfStr.data() + 5, seconds);
    std::from_chars(msfStr.data() + 6, msfStr.data() + 8, frames);

    return (minutes * 60 * 75) + (seconds * 75) + frames;
}

// ------------------------------------------------------------------------------------------------
Disc CueParser::parse(std::string_view rootFolder, std::string_view cueContent)
{
    Disc map;
    map.rootFolder = std::string(rootFolder);
    
    std::string line;
    std::istringstream stream{std::string(cueContent)};

    Track currentTrack;
    std::string currentBinFile = "";
    bool hasTrack = false;
    size_t absoluteLbaAccumulator = 0;

    while (std::getline(stream, line)) {
        std::string_view svLine(line);
    
        // 1. Rimuove gli spazi e i ritorni a capo iniziali
        size_t firstNonSpace = svLine.find_first_not_of(" \t\r\n");
        if (firstNonSpace == std::string_view::npos) continue;
        svLine = svLine.substr(firstNonSpace);

        // 2. CORREZIONE: Rimuove gli spazi e i caratteri \r finali
        size_t lastNonSpace = svLine.find_last_not_of(" \t\r\n");
        if (lastNonSpace != std::string_view::npos) {
            svLine = svLine.substr(0, lastNonSpace + 1);
        }

        if (svLine.starts_with("FILE")) {
            // Se avevamo già una traccia attiva prima di cambiare FILE, dobbiamo calcolarne la fine
            if (hasTrack) {
                FILINFO fno;
                std::string fullPath = map.rootFolder + currentBinFile;
                size_t fileSectors = 0;
                
                if (f_stat(fullPath.c_str(), &fno) == FR_OK) {
                    fileSectors = fno.fsize / 2352; // Settori grezzi standard CD-ROM
                } else {
                    fileSectors = 10000; // Fallback di sicurezza in caso di errore di lettura hardware
                }
                
                
                currentTrack.endSector = currentTrack.startSector + (fileSectors - currentTrack.localOffsetSector) - 1;
                absoluteLbaAccumulator = currentTrack.endSector + 1;
                map.tracks.push_back(currentTrack);
                hasTrack = false;
            }

            size_t firstQuote = svLine.find('"');
            size_t lastQuote = svLine.rfind('"');
            if (firstQuote != std::string_view::npos && lastQuote != std::string_view::npos) {
                currentBinFile = std::string(svLine.substr(firstQuote + 1, lastQuote - firstQuote - 1));
            }
        }
        else if (svLine.starts_with("TRACK")) {
            currentTrack = Track();
            hasTrack = true;
            
            int trackNum = 0;
            std::from_chars(svLine.data() + 6, svLine.data() + 8, trackNum);
            currentTrack.number = trackNum;
            currentTrack.binFile = new BinFile( currentBinFile ); // Assegna il file corrente alla traccia
            currentTrack.type = (svLine.ends_with("AUDIO")) ? TrackType::Audio : TrackType::Data;
            currentTrack.sectorSize = 2352;
            currentTrack.startSector = absoluteLbaAccumulator; // L'LBA iniziale diventa l'accumulato globale
        }
        else if (svLine.starts_with("INDEX 01")) {
            size_t lastSpace = svLine.rfind(' ');
            if (lastSpace != std::string_view::npos) {
                std::string_view msf = svLine.substr(lastSpace + 1);
                currentTrack.localOffsetSector = msfToLBA(msf);
            }
        }
    }

    // Gestione dell'ultima traccia rimasta fuori dal ciclo di cambio FILE
    if (hasTrack) {
        FILINFO fno;
        std::string fullPath = map.rootFolder + currentBinFile;
        size_t fileSectors = 0;
        if (f_stat(fullPath.c_str(), &fno) == FR_OK) {
            fileSectors = fno.fsize / 2352;
        } else {
            fileSectors = 20000;
        }
        currentTrack.endSector = currentTrack.startSector + (fileSectors - currentTrack.localOffsetSector) - 1;
        map.tracks.push_back(currentTrack);
    }

    return map;
}