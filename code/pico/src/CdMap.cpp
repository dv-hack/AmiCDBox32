// CdMap.cpp
//
#include "CdMap.hpp"

#include "CueParser.hpp"

#include "pico/stdlib.h"
#include "ff.h"

#include <iostream>

// ------------------------------------------------------------------------------------------------
CdMap buildCdMap( std::string_view filename )
{
    const auto fullPath = std::string(filename);
    const size_t lastSlash = fullPath.find_last_of("/\\");
    
    auto rootFolder = fullPath.substr(0, lastSlash != std::string::npos ? lastSlash + 1 : 0);
    if (rootFolder.empty()) rootFolder = "0:/";
    
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
            {
                content.append(buffer, bytesRead);
            }
            f_close(&file);
            
            CdMap cdMap = CueParser::parse(rootFolder,content);
            std::cout << "[System] CUE parsed successfully. Found " << cdMap.tracks.size() << " tracks.\n";
            return cdMap;
        } 
        else 
        {
            std::cout << "[System] Error reading CUE file content.\n";
            return {};
        }
    } 
    else 
    {
        // Se è un file ISO, creiamo una mappa fittizia con una singola traccia dati
        Track t;
        t.number = 1;
        t.type = TrackType::Data;
        t.startSector = 0;
        t.binFilename = rootFolder + "/todo.iso";

        CdMap cdMap;
        cdMap.tracks.push_back(t);
        return cdMap;
    }
}