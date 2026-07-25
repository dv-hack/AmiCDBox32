// DeviceControllerGPIO.cpp
//
#include "DeviceControllerGPIO.hpp"

#include "PwmBuzzerPlayer.hpp"
#include "Version.hpp"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include <iostream>
#include <algorithm>

constexpr uint32_t strobePin = 14;
constexpr uint32_t ejectPin = 15;
constexpr uint32_t dacPin   = 20;

// ------------------------------------------------------------------------------------------------
void DeviceControllerGPIO::init()
{
    stdio_init_all();
    sleep_ms(2000); // Allow USB serial to hook up

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
void DeviceControllerGPIO::update()
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
        _drive.tickAudio();

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
void DeviceControllerGPIO::processConfigCommand(std::string_view commandLine)
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

void DeviceControllerGPIO::processHostCommand(std::string_view commandLine)
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
void DeviceControllerGPIO::cmdVersion()
{
    std::cout << "\n--- AmiCDBox32 System Information ---\n";
    std::cout << "Firmware Version: " << FirmwareVersion << '\n';
    std::cout << "Build Date      : " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Toolchain       : GCC " << __VERSION__ << " (C++20 Target)\n";
    std::cout << "Target Hardware : Raspberry Pi Pico\n";
    std::cout << "-------------------------------------\n\n";
}


// ------------------------------------------------------------------------------------------------
void DeviceControllerGPIO::cmdMount(std::string_view filename)
{
    if (!_sdCard.mount())
    {
        std::cout << "[System] Mount failed: storage not ready.\n";
        return;
    }

    if( !_sdCard.fileExists( std::string(filename) ))
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
void DeviceControllerGPIO::cmdList()
{
    std::cout << "[System] Scanning MicroSD root directory...\n";
    
    auto files = _sdCard.fileList();
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
void DeviceControllerGPIO::cmdEject()
{
    if (_drive.getState() == DriveState::NoMedia) {
        std::cout << "[System] No media is currently mounted.\n";
        return;
    }

    _drive.eject();
    _sdCard.unmount();

    
    _systemMode = SystemMode::Command;
    std::cout << "[System] Switching back to COMMAND MODE.\n";
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerGPIO::cmdPlay(std::string_view args)
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
void DeviceControllerGPIO::cmdStop()
{
    _drive.stop();
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerGPIO::handleStrobeTrigger()
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
void DeviceControllerGPIO::handleEjectTrigger()
{
    std::cout << "[System] Physical Eject Button Pressed.\n";
    cmdEject();
}

// ------------------------------------------------------------------------------------------------
std::string DeviceControllerGPIO::readConsoleInput()
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