// DeviceController.cpp
//
#include "DeviceController.hpp"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include <iostream>
#include <algorithm>

constexpr uint32_t strobePin = 14;
constexpr uint32_t ejectPin = 15;

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
    std::cout << "Ready in COMMAND mode. Type LIST, MOUNT, PLAY, STOP or EJECT.\n";
    std::cout << "================================================\n";
}

// ------------------------------------------------------------------------------------------------
void DeviceController::update()
{
    // 1. Handle Serial Console in Command Mode
    if (_systemMode == SystemMode::Command)
    {
        std::string input = readConsoleInput();
        if (!input.empty())
        {
            processCommand(input);
        }
    }

    // 2. Monitor physical buttons (active-low)
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

    sleep_ms(10);
}

// ------------------------------------------------------------------------------------------------
void DeviceController::processCommand(std::string_view commandLine)
{
    if (commandLine == "LIST")
    {
        cmdList();
    }
    else if (commandLine.starts_with("MOUNT "))
    {
        cmdMount(commandLine.substr(6));
    }
    else if (commandLine == "EJECT")
    {
        cmdEject();
    }
    else if (commandLine.starts_with("PLAY"))
    {
        cmdPlay(commandLine.substr(4)); // Safe even if empty
    }
    else if (commandLine == "STOP")
    {
        cmdStop();
    }
    else
    {
        std::cout << "[System] Unknown command: " << commandLine << "\n";
    }
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdList()
{
    std::cout << "[System] Available virtual images (MOCKED):\n";
    std::cout << "  - game_classic.iso (Data only)\n";
    std::cout << "  - super_game_cd.cue (Multi-track data/audio)\n";
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdMount(std::string_view filename)
{
    if (_drive.mount(filename))
    {
        _systemMode = SystemMode::Storage;
        std::cout << "[System] " << filename << " successfully mounted.\n";
        std::cout << "[System] Switching to STORAGE MODE. Drive is ready for host triggers.\n";
    }
}

// ------------------------------------------------------------------------------------------------
void DeviceController::cmdEject()
{
    _drive.eject();
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