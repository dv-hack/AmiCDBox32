// FileSystem.hpp
//
#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <vector>

namespace FileSystem
{

enum class ErrorCode
{
    NoError = 0,
    FileNotFound,
    FileOperationError,
    EndOfFile
};

struct FileInfo
{
    std::string name;
    uint32_t size{0};
    bool isDirectory{false};
};

class Path
{
public:
    Path(const std::string& p);
    
    std::string string() const { return _path; }
    std::string extension() const;
    std::string filename() const;
    
    Path operator/(const std::string& append) const;

private:
    std::string _path;

};

// ------------------------------------------------------------------------------------------------
class File
{
public:
    File();
    ~File();
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;

    bool isOpen() const;
    void close();
    uint32_t getSize() const;
    
    ErrorCode read(uint8_t* buffer, uint32_t sizeToRead, uint32_t& bytesRead);
    ErrorCode readAll(uint8_t* buffer, uint32_t sizeToRead);
    ErrorCode seek(uint32_t offset);

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;

    friend File open( const std::string &);
};

// ------------------------------------------------------------------------------------------------
class Dir
{
public:
    Dir();
    ~Dir();

    Dir(const Dir&) = delete;
    Dir& operator=(const Dir&) = delete;
    Dir(Dir&& other) noexcept;
    Dir& operator=(Dir&& other) noexcept;

    bool isOpen() const;
    void close();
    bool readNext(FileInfo& outInfo);

    bool hasNext() const;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;

    friend Dir openDir(const std::string& );
};

// ------------------------------------------------------------------------------------------------
bool     init();
File     open(const std::string& filename);
Dir      openDir(const std::string& path);
FileInfo stat(const std::string& filename);
Path     makePath(const std::string& path);
bool     exists( const std::string& path );

} // namespace FileSystem