// BinFile.hpp
//
#pragma once

#include "Sector.hpp"

#include <string_view>
#include <cstdint>
#include <memory>


class BinFile
{
public:
    enum class ErrorCode
    {
        NoError,
        FileNotFound,
        FileOperationError,
        AlreadyOpened,
        EndOfFile
    };

public:
    BinFile();
    ~BinFile();
    BinFile(const BinFile&) = delete;
    BinFile& operator=(const BinFile&) = delete;
    BinFile(BinFile&&) noexcept;
    BinFile& operator=(BinFile&&) noexcept;    

    ErrorCode open( std::string_view filename );
    ErrorCode close();
    ErrorCode readSectorAt( size_t lba, Sector &s );

    bool isOpen() const { return _file != nullptr;}

private:   
    struct BinFileRaw;
    std::unique_ptr<BinFileRaw> _file;
};