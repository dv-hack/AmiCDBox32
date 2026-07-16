// DeviceController.hpp
//
#pragma once

#include "CdDrive.hpp"
#include <string>
#include <string_view>

enum class SystemMode
{
    Command,
    Storage
};

class DeviceController
{
public:
    DeviceController() = default;

    void init();
    void update();

private:
    void processCommand(std::string_view commandLine);

    // Command Handlers
    void cmdList();
    void cmdMount(std::string_view filename);
    void cmdEject();
    void cmdPlay(std::string_view args);
    void cmdStop();

    // Hardware Event Handlers
    void handleStrobeTrigger();
    void handleEjectTrigger();

    // Console Helper
    std::string readConsoleInput();

    SystemMode _systemMode = SystemMode::Command;
    CdDrive _drive;

    bool _lastStrobeState = true;
    bool _lastEjectState = true;
};