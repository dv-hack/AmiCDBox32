// FileSystem.PICO.cpp
//
#include "FileSystem.hpp"

#include "LedDebugger.hpp"

#include "spi.h"
#include "sd_card.h"
#include "diskio.h"
#include "ff.h"

#include <iostream>
#include <string>

// 1. Configurazione del blocco hardware SPI (SPI 0)
static spi_t spis[] = {
    {
        .hw_inst = spi0,           // Istanza hardware del Pico
        .miso_gpio = 16,           // GP16
        .mosi_gpio = 19,           // GP19
        .sck_gpio = 18,            // GP18
        .baud_rate = 12500 * 1000, // 12.5 MHz (ottimale per SD)
        .set_drive_strength = false
    }
};

// 2. Configurazione della singola scheda SD
static sd_card_t sd_cards[] = {
    {
        .pcName = "0:",            // Nome del volume logico per FatFS
        .spi = &spis[0],           // Punta all'istanza SPI sopra
        .ss_gpio = 17,             // Chip Select (CS) su GP17
        .use_card_detect = false,   // Non usiamo il pin CD fisico del lettore
        .card_detect_gpio = 0,
        .card_detected_true = 0,
        .m_Status = STA_NOINIT,
        .mounted = false
    }
};

extern "C" size_t spi_get_num() { return sizeof(spis) / sizeof(spis[0]); }
extern "C" spi_t *spi_get_by_num(size_t num) {
    if (num < spi_get_num()) return &spis[num];
    return nullptr;
}

extern "C" size_t sd_get_num() { return sizeof(sd_cards) / sizeof(sd_cards[0]); }
extern "C" sd_card_t *sd_get_by_num(size_t num) {
    if (num < sd_get_num()) return &sd_cards[num];
    return nullptr;
}


namespace FileSystem
{
    static FATFS fs; // File system object for FATFS

    struct File::Impl
    {
        FIL        file;
        uint32_t    size{0};
    };

    struct Dir::Impl
    {
        DIR dir;
        FILINFO entry;
        bool hasNext{false};

        // prefetch helper to read the next entry in the directory, skipping "." and ".."
        static bool readNextEntry(Impl &impl)
        {
            while (true)
            {
                FRESULT res = f_readdir(&impl.dir, &impl.entry);
                if (res != FR_OK || impl.entry.fname[0] == '\0')
                {
                    impl.hasNext = false;
                    return false;
                }

                std::string_view name(impl.entry.fname);
                if (name == "." || name == "..")
                    continue;

                impl.hasNext = true;
                return true;
            }
        }
    };

    // Helper function to normalize paths for FatFs
    static std::string normalizeFatPath(const std::string &path)
    {
        if (path.empty())               return "0:/";
        if (path.rfind("0:/", 0) == 0)  return path;
        if (path.rfind("0:", 0) == 0)   return path[2] == '/' ? path : "0:/" + path.substr(2);
        if (path[0] == '/')             return "0:" + path;
        return "0:/" + path;
    }    


    bool init()
    {
        if( fs.fs_type != 0 ) // Check if already mounted
            return true;

        // FfatFs mount: volume 0, 1 = force mount immediately to verify hardware
        const auto result = f_mount(&fs, "0:", 1);
        if (result != FR_OK) 
        {
            std::cout << "[Storage] Failed to mount FAT32 File System. Error code: " << static_cast<int>(result) << "\n";
            return false;
        }

        std::cout << "[Storage] FAT32 File System successfully mounted.\n";
        return true;
    }

    // ============================================================================
    // Path
    // ============================================================================
    Path::Path(const std::string& p) 
    : _path(normalizeFatPath(p)) 
    {}

    std::string Path::extension() const
    {
        const auto pos = _path.rfind('.');
        return pos == std::string::npos
            ? "" 
            : _path.substr(pos);
    }

    std::string Path::filename() const
    {
        const auto pos = _path.rfind('/');
        return pos == std::string::npos
            ? _path 
            : _path.substr(pos + 1);
    }

    Path Path::operator/(const std::string &append) const
    {
        // concatenate current path with the provided string using path separator character
        const FileInfo info = stat(_path);
        return info.isDirectory
            ? Path(_path + "/" + append)
            : Path(_path.substr(0, _path.rfind('/')) + "/" + append);
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
        return _impl != nullptr;
    }

    void File::close()
    {
        if ( isOpen() )
        {
            f_close(&_impl->file);
            _impl.reset();
        }   
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

        f_read(&_impl->file, buffer, sizeToRead, reinterpret_cast<UINT*>(&bytesRead));
        if (bytesRead < sizeToRead)
        {
            if (_impl->file.err == FR_OK && f_eof(&_impl->file))
                return ErrorCode::EndOfFile;
            return ErrorCode::FileOperationError;
        }

        return ErrorCode::NoError;
    }

    ErrorCode File::readAll(uint8_t *buffer, uint32_t sizeToRead)
    {
        if (!isOpen())
            return ErrorCode::FileOperationError;

        uint32_t bytesRead = 0;
        f_read(&_impl->file, buffer, sizeToRead,reinterpret_cast<UINT*>(&bytesRead));

        if (bytesRead < sizeToRead)
        {
            if (_impl->file.err == FR_OK && f_eof(&_impl->file))
                return ErrorCode::EndOfFile;
            return ErrorCode::FileOperationError;
        }

        return ErrorCode::NoError;
    }

    ErrorCode File::seek(uint32_t offset)
    {
        if (!isOpen())
            return ErrorCode::FileOperationError;

        const auto ok = f_lseek(&_impl->file, static_cast<FSIZE_t>(offset));
        return ok == FR_OK 
            ? ErrorCode::NoError 
            : ErrorCode::FileOperationError;
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
        if (isOpen())
        {
            f_closedir(&_impl->dir);
            _impl.reset();
        }   
    }

    bool Dir::hasNext() const
    {
        return _impl && _impl->hasNext;
    }

    bool Dir::readNext(FileInfo &outInfo)
    {
        if (!isOpen() || !_impl->hasNext)
            return false;

        outInfo.name = _impl->entry.fname;
        outInfo.isDirectory = (_impl->entry.fattrib & AM_DIR) != 0;
        outInfo.size = outInfo.isDirectory ? 0 : static_cast<uint32_t>(_impl->entry.fsize);

        // prefetch the next entry for subsequent calls
        return Impl::readNextEntry(*_impl);
    }

    // ============================================================================
    // Global functions
    // ============================================================================
    File open(const std::string &filename)
    {
        const std::string normalized = normalizeFatPath(filename);

        File f;
        f._impl = std::make_unique<File::Impl>();
        const auto result = f_open(&f._impl->file, normalized.c_str(), FA_READ);
        if (result == FR_OK)
        {
            f._impl->size = static_cast<uint32_t>(f_size(&f._impl->file));
        }
        else
        {
            std::cout << "[FileSystem] Error: Cannot open file: " << normalized << " Error code: " << static_cast<int>(result) << "\n";
            f._impl.reset();
        }
        return f;
    }

    Dir openDir(const std::string &path)
    {
        const std::string normalized = normalizeFatPath(path);

        Dir d;
        d._impl = std::make_unique<Dir::Impl>();
        const auto result = f_opendir(&d._impl->dir, normalized.c_str());
        if (result != FR_OK)
        {
            std::cout << "[FileSystem] Error: Cannot open directory: " << normalized << " Error code: " << static_cast<int>(result) << "\n";
            d._impl.reset();
        }

        Dir::Impl::readNextEntry(*d._impl); // prefetch the first entry
        return d;
    }

    FileInfo stat(const std::string &filename)
    {
        const std::string normalized = normalizeFatPath(filename);

        FILINFO fno;
        if (f_stat(normalized.c_str(), &fno) != FR_OK)
            return {};

        FileInfo info;
        info.name = fno.fname;
        info.isDirectory = (fno.fattrib & AM_DIR) != 0;
        info.size = info.isDirectory ? 0 : static_cast<uint32_t>(fno.fsize);
        return info;
    }

    Path makePath(const std::string &path)
    {
        return Path(path);
    }

    bool exists( const std::string& path )
    {
        const std::string normalized = normalizeFatPath(path);
        FILINFO fno;
        return f_stat(normalized.c_str(), &fno) == FR_OK;
    }
}