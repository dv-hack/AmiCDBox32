// SDCardHandler.cpp
//
#include "SDCardHandler.hpp"

#include "ff.h"

#include <iostream>
#include <algorithm>
#include <string_view>

// ------------------------------------------------------------------------------------------------
struct SDCardHandler::FileSystem
{
    FATFS fs;
};

// ------------------------------------------------------------------------------------------------
bool SDCardHandler::mount()
{
    if( _fileSystem )
        return true;

    _fileSystem = std::make_unique<FileSystem>();

    // FfatFs mount: volume 0, 1 = force mount immediately to verify hardware
    FRESULT result = f_mount(&_fileSystem->fs, "0:", 1);
    if (result != FR_OK) 
    {
        std::cout << "[Storage] Failed to mount FAT32 File System. Error code: " << static_cast<int>(result) << "\n";
        return false;
    }

    std::cout << "[Storage] FAT32 File System successfully mounted.\n";
    return true;        
}

// ------------------------------------------------------------------------------------------------
void SDCardHandler::unmount()
{
    if( !_fileSystem )
        return;

    // unmount fatfs
    f_mount(nullptr, "0:", 0);
    _fileSystem.reset();
}

// ------------------------------------------------------------------------------------------------
bool SDCardHandler::fileExists( const std::string &filename ) const
{
    if( !_fileSystem )
        return false;

    std::string fullPath = filename;
    if( !fullPath.starts_with("0:/"))
        fullPath = "0:/" + std::string(filename);
    
    // check if exists
    FILINFO fileInfo;
    return f_stat(fullPath.c_str(), &fileInfo) == FR_OK;
}


// ------------------------------------------------------------------------------------------------
std::vector<std::string> SDCardHandler::fileList() const
{
    if (!_fileSystem)
        return {};
    
    DIR rootDir;
    FILINFO fileInfo;
    std::vector<std::string> files;
    
    if (f_opendir(&rootDir, "0:/") != FR_OK) 
    {
        std::cout << "[Storage] Cannot open root directory.\n";
        return {};
    }

    while (f_readdir(&rootDir, &fileInfo) == FR_OK && fileInfo.fname[0] != 0)
    {
        std::string_view fname(fileInfo.fname);
        
        if (fileInfo.fattrib & AM_DIR) 
        {
            // skip folder references
            if (fname == "." || fname == "..") 
                continue;
            
            // open subfolder
            DIR subDir;
            const std::string subPath = "0:/" + std::string(fname);
            if (f_opendir(&subDir, subPath.c_str()) == FR_OK) 
            {
                FILINFO subFileInfo;
                while (f_readdir(&subDir, &subFileInfo) == FR_OK && subFileInfo.fname[0] != 0) 
                {
                    if (!(subFileInfo.fattrib & AM_DIR)) 
                    {
                        const std::string_view subFname(subFileInfo.fname);
                        // Filtriamo per estensioni valide (case-insensitive di base)
                        if (subFname.ends_with(".cue") || subFname.ends_with(".CUE") || 
                            subFname.ends_with(".iso") || subFname.ends_with(".ISO")) 
                        {
                            files.push_back(std::string(fname) + "/" + subFileInfo.fname);
                        }
                    }
                }
                f_closedir(&subDir);
            }
        } 
        else 
        {
            // list any iso/cue in root folder
            if (fname.ends_with(".cue") || fname.ends_with(".CUE") || 
                fname.ends_with(".iso") || fname.ends_with(".ISO")) 
            {
                files.push_back(fileInfo.fname);
            }
        }
    }

    f_closedir(&rootDir);
    return files;
}