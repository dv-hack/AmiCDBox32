// BinFile.cpp
//
#include "BinFile.hpp"

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
BinFile::BinFile() = default;
BinFile::~BinFile() = default;
BinFile::BinFile(BinFile&&) noexcept = default;
BinFile& BinFile::operator=(BinFile&&) noexcept = default;

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
    
    if (bytesRead != s.size )
    {
        std::cout << "[BinFile] End of BIN file reached.\n";
        return ErrorCode::EndOfFile;
    }

    return ErrorCode::NoError;
}