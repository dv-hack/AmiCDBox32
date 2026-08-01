// FileSystem.PC.cpp
//
#include "FileSystem.hpp"

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace FileSystem
{
    struct File::Impl
    {
        std::ifstream stream;
        uint32_t size{0};
    };

    struct Dir::Impl
    {
        fs::directory_iterator iterator;
        fs::directory_iterator end;
    };

    bool init()
    {
        // For PC, no special initialization is needed.
        return true;
    }

    // ============================================================================
    // Path
    // ============================================================================
    std::string Path::extension() const
    {
        return fs::path(_path).extension().string();
    }

    std::string Path::filename() const
    {
        return fs::path(_path).filename().string();
    }

    Path Path::operator/(const std::string &append) const
    {
        fs::path p(_path);
        p /= append;
        return Path(p.string());
    }

    // ============================================================================
    // File
    // ============================================================================
    File::File() = default;
    File::~File() { close(); }

    File::File(File &&other) noexcept = default;
    File &File::operator=(File &&other) noexcept = default;

    bool File::isOpen() const
    {
        return _impl && _impl->stream.is_open();
    }

    void File::close()
    {
        if (_impl && _impl->stream.is_open())
            _impl->stream.close();
    }

    uint32_t File::getSize() const
    {
        return _impl ? _impl->size : 0;
    }

    ErrorCode File::read(uint8_t *buffer, uint32_t sizeToRead, uint32_t &bytesRead)
    {
        if (!isOpen())
        {
            bytesRead = 0;
            return ErrorCode::FileOperationError;
        }

        _impl->stream.read(reinterpret_cast<char *>(buffer), sizeToRead);
        bytesRead = static_cast<uint32_t>(_impl->stream.gcount());

        if (bytesRead < sizeToRead)
        {
            if (_impl->stream.eof())
                return ErrorCode::EndOfFile;
            return ErrorCode::FileOperationError;
        }

        return ErrorCode::NoError;
    }

    ErrorCode File::seek(uint32_t offset)
    {
        if (!isOpen())
            return ErrorCode::FileOperationError;

        _impl->stream.clear(); // Pulisce eventuali flag di EOF prima di muoversi
        _impl->stream.seekg(offset, std::ios::beg);

        if (_impl->stream.fail())
            return ErrorCode::FileOperationError;
        return ErrorCode::NoError;
    }

    // ============================================================================
    // Dir
    // ============================================================================
    Dir::Dir() = default;
    Dir::~Dir() = default;
    Dir::Dir(Dir &&other) noexcept = default;
    Dir &Dir::operator=(Dir &&other) noexcept = default;

    bool Dir::isOpen() const
    {
        return _impl != nullptr;
    }

    void Dir::close()
    {
        _impl.reset();
    }

    bool Dir::hasNext() const
    {
        return isOpen() && _impl->iterator != _impl->end;
    }

    bool Dir::readNext(FileInfo &outInfo)
    {
        if (!isOpen() || _impl->iterator == _impl->end)
            return false;

        const auto &entry = *_impl->iterator;
        outInfo.name = entry.path().filename().string();
        outInfo.isDirectory = entry.is_directory();

        if (outInfo.isDirectory)
            outInfo.size = 0;
        else
        {
            std::error_code ec;
            outInfo.size = static_cast<uint32_t>(fs::file_size(entry.path(), ec));
        }

        ++_impl->iterator;
        return true;
    }

    // ============================================================================
    // Global functions
    // ============================================================================
    File open(const std::string &filename)
    {
        File f;
        f._impl = std::make_unique<File::Impl>();
        f._impl->stream.open(filename, std::ios::binary | std::ios::in);

        if (f._impl->stream.is_open())
        {
            f._impl->stream.seekg(0, std::ios::end);
            f._impl->size = static_cast<uint32_t>(f._impl->stream.tellg());
            f._impl->stream.seekg(0, std::ios::beg);
        }
        else
        {
            f._impl.reset();
        }
        return f;
    }

    Dir openDir(const std::string &path)
    {
        Dir d;
        std::error_code ec;
        if (fs::is_directory(path, ec))
        {
            d._impl = std::make_unique<Dir::Impl>();
            d._impl->iterator = fs::directory_iterator(path, ec);
        }
        return d;
    }

    FileInfo stat(const std::string &filename)
    {
        FileInfo info;
        std::error_code ec;

        if (!fs::exists(filename, ec))
            return info;

        info.name = fs::path(filename).filename().string();
        info.isDirectory = fs::is_directory(filename, ec);
        info.size = info.isDirectory ? 0 : static_cast<uint32_t>(fs::file_size(filename, ec));

        return info;
    }

    Path makePath(const std::string &path)
    {
        return Path(path);
    }

    bool exists( const std::string& path )
    {
        std::error_code ec;
        return fs::exists( path, ec);
    }


}