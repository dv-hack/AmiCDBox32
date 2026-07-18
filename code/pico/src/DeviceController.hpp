// DeviceController.hpp
//
#pragma once

#include "CdDrive.hpp"

#include "ff.h"

#include <string>
#include <vector>
#include <string_view>

enum class SystemMode
{
    Command,
    Storage
};

static constexpr char FirmwareVersion[] = "0.3.0-alpha (FatFs Native Integration)";

class DeviceController
{
public:
    DeviceController() = default;

    void init();
    void update();

private:
    void processCommand(std::string_view commandLine);
    bool initializeFileSystem();
    std::vector<std::string> fileList();

    // Command Handlers
    void cmdList();
    void cmdMount(std::string_view filename);
    void cmdEject();
    void cmdPlay(std::string_view args);
    void cmdStop();
    void cmdVersion();

    // Hardware Event Handlers
    void handleStrobeTrigger();
    void handleEjectTrigger();

    // Console Helper
    std::string readConsoleInput();

private:
    SystemMode _systemMode = SystemMode::Command;
    CdDrive _drive;
    FATFS _fileSystem;

    bool _sdInitialized = false;
    bool _lastStrobeState = true;
    bool _lastEjectState = true;
};