// BinFile.cpp
//
#include "BinFile.hpp"

#include "LedDebugger.hpp"

#include "Sector.hpp"

#include "ff.h"

#include <iostream>
#include <string>

// ------------------------------------------------------------------------------------------------
struct BinFile::BinFileRaw
{
    std::string filename;
    FIL file;
};

// ------------------------------------------------------------------------------------------------
BinFile::BinFile( std::string_view filename ) { open( filename );}
BinFile::~BinFile() { close();}

// ------------------------------------------------------------------------------------------------
BinFile::ErrorCode BinFile::open( std::string_view filename )
{
    if (_file)
        return ErrorCode::AlreadyOpened;

    _file = std::make_unique<BinFileRaw>();
    _file->filename = filename;

    if (f_open(&(_file->file), _file->filename.c_str(), FA_READ) != FR_OK) 
    {
        std::cout << "[BinFile] Error: Cannot open BIN file: " << _file->filename << "\n";
        return ErrorCode::FileNotFound;
    }

    return ErrorCode::NoError;
}

// ------------------------------------------------------------------------------------------------
BinFile::ErrorCode BinFile::close()
{
    if( _file == nullptr )
        return ErrorCode::NoError;

    if( f_close(&_file->file) != FR_OK )
    {
        std::cout << "[BinFile] Error: Cannot close BIN file: " << _file->filename << "\n";
        return ErrorCode::FileOperationError;
    }   
    _file.reset();
    return ErrorCode::NoError;
}

// ------------------------------------------------------------------------------------------------
uint32_t BinFile::totalSectors() const
{
    if( _file == nullptr )
        return 0;
    FSIZE_t size = f_size(&_file->file); 
    return static_cast<uint32_t>(size / 2048);
}

// ------------------------------------------------------------------------------------------------
BinFile::ErrorCode BinFile::readSectorAt( size_t lba, Sector &s )
{
    if( _file == nullptr )
    {
        std::cout << "[BinFile] Error: Cannot read sector. File is not opened.\n";
        return ErrorCode::FileOperationError;
    }

    const FSIZE_t offset = static_cast<FSIZE_t>(lba) * s.size;
    f_lseek(&_file->file, offset);

    
    UINT bytesRead;    
    f_read(&_file->file, &s.bytes[0], s.size, &bytesRead);

    if( bytesRead == 2353 )
        setActivityLed(false);

    if( bytesRead >0 && bytesRead < s.size )
        setActivityLed(false);
    
    if (bytesRead != s.size )
    {
        std::cout << "[BinFile] End of BIN file reached.\n";
        setErrorLed(true);
        return ErrorCode::EndOfFile;
    }

    return ErrorCode::NoError;
}