// DeviceController.cpp
//
#include "DeviceController.hpp"

#include "PwmBuzzerPlayer.hpp"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include <iostream>
#include <algorithm>

constexpr uint32_t strobePin = 14;
constexpr uint32_t ejectPin = 15;
constexpr uint32_t dacPin   = 20;

// ------------------------------------------------------------------------------------------------
void DeviceController::init()
{
    gpio_init(strobePin);
    gpio_set_dir(strobePin, GPIO_IN);
    gpio_pull_up(strobePin);

    gpio_init(ejectPin);
    gpio_set_dir(ejectPin, GPIO_IN);
    gpio_pull_up(ejectPin);

    std::cout << "\n=== AmiCDBox32 System Controller Initialized ===\n";
    std::cout << "Ready in COMMAND mode. Type VERSION, LIST, MOUNT, PLAY, STOP or EJECT.\n";
    std::cout << "================================================\n";

    _drive = CdDrive( new PwmBuzzerPlayer(dacPin));
}

// ------------------------------------------------------------------------------------------------
void DeviceController::update()
{
    // 1. Handle Serial Console in Command Mode
    const auto input = readConsoleInput();
    if( !input.empty() )
    {
        if ( input == "EJECT" )
        {
            cmdEject();
        }
        else if (_systemMode == SystemMode::Command) 
        {
            processConfigCommand(input);
        }
        else if (_systemMode == SystemMode::Storage) 
        {
            processHostCommand(input);
        }
    }
  

    // 2. Autonomous time management for AUDIO streaming (CD-DA at 75Hz)
    if (_drive.getState() == DriveState::PlayingAudio)
    {
        _drive.tickAudio();
        // static uint64_t nextSectorTimeUs = 0;
        // uint64_t currentTimeUs = time_us_64();

        // if (currentTimeUs >= nextSectorTimeUs)
        // {
        //     // Set deadline for the next sector (~13.33 ms)
        //     nextSectorTimeUs = currentTimeUs + 13333;
        //     _drive.tickAudio();
        // }
    }

    // 3. Monitor physical buttons (active-low)
    bool currentStrobe = gpio_get(strobePin);
    bool currentEject = gpio_get(ejectPin);

    if (!currentStrobe && _lastStrobeState)
    {
        handleStrobeTrigger();
        sleep_ms(150); // Debounce
    }

    if (!currentEject && _lastEjectState)
    {
        handleEjectTrigger();
        sleep_ms(150); // Debounce
    }

    _lastStrobeState = currentStrobe;
    _lastEjectState = currentEject;

    sleep_ms(1); // Keep loop fast for tight audio audio tick polling
}

// ------------------------------------------------------------------------------------------------
void DeviceController::processConfigCommand(std::string_view commandLine)
{
    if (commandLine == "LIST") {
        cmdList();
    } else if (commandLine.starts_with("MOUNT ")) {
        cmdMount(commandLine.substr(6));
    } else if (commandLine == "VERSION") {
        cmdVersion();
    } else {
        std::cout << "[System] Invalid command. Available in CONFIG mode: LIST, MOUNT, EJECT, VERSION.\n";
    }
}

void DeviceController::processHostCommand(std::string_view commandLine)
{
    if (commandLine.starts_with("PLAY")) {
        cmdPlay(commandLine.substr(4));
    } else if (commandLine == "STOP") {
        cmdStop();
    } else {
        std::cout << "[Amiga Bus] Ignored. Available in STORAGE mode: PLAY, STOP, EJECT.\n";
    }
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdVersion()
{
    std::cout << "\n--- AmiCDBox32 System Information ---\n";
    std::cout << "Firmware Version: " << FirmwareVersion << '\n';
    std::cout << "Build Date      : " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Toolchain       : GCC " << __VERSION__ << " (C++20 Target)\n";
    std::cout << "Target Hardware : Raspberry Pi Pico\n";
    std::cout << "-------------------------------------\n\n";
}


// ------------------------------------------------------------------------------------------------
void DeviceController::cmdMount(std::string_view filename)
{
    if (!initializeFileSystem())
    {
        std::cout << "[System] Mount failed: storage not ready.\n";
        return;
    }

    // absolute file path for f_stat
    const std::string fullPath = "0:/" + std::string(filename);
    
    // check if exists
    FILINFO fileInfo;
    if (f_stat(fullPath.c_str(), &fileInfo) != FR_OK)
    {
        std::cout << "[System] Error: File '" << filename << "' not found on MicroSD.\n";
        return;
    }

    // the provided filename can be mounted
    if (_drive.mount(filename))
    {
        _systemMode = SystemMode::Storage;
        std::cout << "[System] " << filename << " successfully mounted.\n";
        std::cout << "[System] Switching to STORAGE MODE. Drive is ready for host triggers.\n";
    }
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdList()
{
    std::cout << "[System] Scanning MicroSD root directory...\n";
    
    auto files = fileList();
    if (files.empty()) 
    {
        std::cout << "[System] No files found or SD card unreadable.\n";
        return;
    }

    std::cout << "--- Files found on MicroSD ---\n";
    for (const auto& file : files) {
        std::cout << "  -> " << file << "\n";
    }
    std::cout << "------------------------------\n";
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdEject()
{
    if (_drive.getState() == DriveState::NoMedia) {
        std::cout << "[System] No media is currently mounted.\n";
        return;
    }

    _drive.eject();

    // unmount fatfs
    f_mount(nullptr, "0:", 0);
    _sdInitialized = false;
    
    _systemMode = SystemMode::Command;
    std::cout << "[System] Switching back to COMMAND MODE.\n";
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdPlay(std::string_view args)
{
    if (_drive.getState() == DriveState::NoMedia)
    {
        std::cout << "[System] Error: No disc mounted.\n";
        return;
    }

    // Strip optional spaces
    size_t firstNonSpace = args.find_first_not_of(" ");
    if (firstNonSpace != std::string_view::npos)
    {
        std::string_view argVal = args.substr(firstNonSpace);
        // If argument looks like a track number (e.g. TRK02 or just a number < 99)
        if (argVal.size() <= 2)
        {
            int track = std::stoi(std::string(argVal));
            _drive.playTrack(track);
        }
        else
        {
            size_t lba = std::stoull(std::string(argVal));
            _drive.play(lba);
        }
    }
    else
    {
        // Play from current LBA if no arguments provided
        _drive.play(_drive.getCurrentLba());
    }
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdStop()
{
    _drive.stop();
}

// ------------------------------------------------------------------------------------------------
void DeviceController::handleStrobeTrigger()
{
    if (_systemMode == SystemMode::Storage)
    {
        _drive.advanceSector();
    }
    else
    {
        std::cout << "[System] Host strobe ignored (Not in Storage Mode).\n";
    }
}

// ------------------------------------------------------------------------------------------------
void DeviceController::handleEjectTrigger()
{
    std::cout << "[System] Physical Eject Button Pressed.\n";
    cmdEject();
}

// ------------------------------------------------------------------------------------------------
std::string DeviceController::readConsoleInput()
{
    static std::string buffer = "";
    int character = getchar_timeout_us(0);
    while (character != PICO_ERROR_TIMEOUT)
    {
        if (character == '\r' || character == '\n')
        {
            std::string line = buffer;
            buffer.clear();
            return line;
        }
        else
        {
            buffer += static_cast<char>(character);
        }
        character = getchar_timeout_us(0);
    }
    return "";
}

// ------------------------------------------------------------------------------------------------
bool DeviceController::initializeFileSystem()
{
    if( _sdInitialized )
        return true;

    // FfatFs mount: volume 0, 1 = force mount immediately to verify hardware
    FRESULT result = f_mount(&_fileSystem, "0:", 1);
    if (result != FR_OK) 
    {
        std::cout << "[Storage] Failed to mount FAT32 File System. Error code: " << static_cast<int>(result) << "\n";
        return false;
    }

    std::cout << "[Storage] FAT32 File System successfully mounted.\n";
    _sdInitialized = true;
    return true;        
}

// ------------------------------------------------------------------------------------------------
std::vector<std::string> DeviceController::fileList()
{
if (!initializeFileSystem())
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