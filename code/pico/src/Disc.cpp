// Disc.cpp
//
#include "Disc.hpp"

#include "CueParser.hpp"
#include "LedDebugger.hpp"

#include "pico/stdlib.h"
#include "ff.h"

#include <iostream>

// ------------------------------------------------------------------------------------------------
Disc Disc::mount( std::string_view filename )
{
    const auto fullPath = std::string(filename);
    const size_t lastSlash = fullPath.find_last_of("/\\");
    
    auto rootFolder = fullPath.substr(0, lastSlash != std::string::npos ? lastSlash + 1 : 0);
    if (rootFolder.empty())
        rootFolder = "0:/";
    
    // Se è un file CUE, lo leggiamo e lo parsiamo
    if (filename.ends_with(".cue") || filename.ends_with(".CUE")) 
    {
        FIL file;
        
        if (f_open(&file, fullPath.c_str(), FA_READ) == FR_OK) 
        {
            std::string content;
            char buffer[256];
            UINT bytesRead;
            
            // Legge il file a blocchi di 256 byte fino alla fine
            while (f_read(&file, buffer, sizeof(buffer), &bytesRead) == FR_OK && bytesRead > 0) 
                content.append(buffer, bytesRead);
            f_close(&file);
            
            Disc disc = CueParser::parse(rootFolder,content);
            std::cout << "[System] CUE parsed successfully. Found " << disc.tracks.size() << " tracks.\n";
            return disc;
        } 
        else 
        {
            std::cout << "[System] Error reading CUE file content.\n";
            return {};
        }
    } 
    else 
    {
        Track t;
        t.number = 1;
        t.type = TrackType::Data;
        t.startSector = 0; 
        t.localOffsetSector = 0;
        //t.fileHeaderOffset = 0;
        t.binFile = new BinFile(rootFolder + std::string(filename) );
        t.sectorSize = (filename.ends_with(".iso") || filename.ends_with(".ISO")) ? 2048 : 2352;
        
        // Chiediamo a FatFs quanto è grande il file per calcolare i settori totali
        FILINFO fno;
        size_t fileSectors = 0;
        if (f_stat(fullPath.c_str(), &fno) == FR_OK) {
            fileSectors = fno.fsize / t.sectorSize;
        } else {
            std::cout << "[System] Warning: Could not stat file " << fullPath << ".\n";
            fileSectors = 10000; // Fallback di sicurezza
        }

        t.endSector = (fileSectors > 0) ? fileSectors - 1 : 0;        

        Disc cdMap;
        cdMap.rootFolder = rootFolder;
        cdMap.tracks.push_back(t);
        return cdMap;
    }
}

uint32_t Disc::getTotalSectors() const
{
    
}
