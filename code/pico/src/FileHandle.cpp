// FileHandle.cpp
//
#include "FileHandle.hpp"

#include "LedDebugger.hpp"

#include "Sector.hpp"

#include "ff.h"

#include <iostream>
#include <string>

// ------------------------------------------------------------------------------------------------
struct FileHandle::FileHandleRaw
{
    std::string filename;
    FIL file;
};

// ------------------------------------------------------------------------------------------------
FileHandle::FileHandle( std::string_view filename ) { open( filename );}
FileHandle::~FileHandle() { close();}

// ------------------------------------------------------------------------------------------------
FileHandle::ErrorCode FileHandle::open( std::string_view filename )
{
    if (_file)
        return ErrorCode::AlreadyOpened;

    _file = std::make_unique<FileHandleRaw>();
    _file->filename = filename;

    if (f_open(&(_file->file), _file->filename.c_str(), FA_READ) != FR_OK) 
    {
        std::cout << "[FileHandle] Error: Cannot open BIN file: " << _file->filename << "\n";
        return ErrorCode::FileNotFound;
    }

    return ErrorCode::NoError;
}

// ------------------------------------------------------------------------------------------------
FileHandle::ErrorCode FileHandle::close()
{
    if( _file == nullptr )
        return ErrorCode::NoError;

    if( f_close(&_file->file) != FR_OK )
    {
        std::cout << "[FileHandle] Error: Cannot close BIN file: " << _file->filename << "\n";
        return ErrorCode::FileOperationError;
    }   
    _file.reset();
    return ErrorCode::NoError;
}

// ------------------------------------------------------------------------------------------------
uint32_t FileHandle::size() const
{
    if( _file == nullptr )
        return 0;
    FSIZE_t size = f_size(&_file->file); 
    return static_cast<uint32_t>(size);
}

// ------------------------------------------------------------------------------------------------
FileHandle::ErrorCode FileHandle::read( size_t offset, uint8_t* bytes, size_t bytesToRead )
{
    if( _file == nullptr )
    {
        std::cout << "[FileHandle] Error: Cannot read sector. File is not opened.\n";
        return ErrorCode::FileOperationError;
    }

    f_lseek(&_file->file, static_cast<FSIZE_t>(offset));
    
    UINT bytesRead;    
    f_read(&_file->file, bytes, bytesToRead, &bytesRead);
    
    if (bytesRead != bytesToRead )
    {
        std::cout << "[FileHandle] End of BIN file reached.\n";
        setErrorLed(true);
        return ErrorCode::EndOfFile;
    }

    return ErrorCode::NoError;
}