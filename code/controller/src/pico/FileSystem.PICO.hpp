// FileHandle.hpp
//
#pragma once

#include <string_view>
#include <cstdint>
#include <memory>


class FileHandle
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
    explicit FileHandle( std::string_view filename );
    FileHandle() = default;
    ~FileHandle();
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&&) noexcept = default;
    FileHandle& operator=(FileHandle&&) noexcept = default;

    ErrorCode open( std::string_view filename );
    ErrorCode close();
    ErrorCode read( size_t lba, uint8_t* bytes, size_t bytesToRead );

    bool isOpen() const { return _file != nullptr;}
    uint32_t size() const;

private:   
    struct FileHandleRaw;
    std::unique_ptr<FileHandleRaw> _file;
};