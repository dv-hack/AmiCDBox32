// DeviceControllerGPIO.hpp
//
#pragma once

#include "DeviceController.hpp"
#include "CdDrive.hpp"
#include "SDCardHandler.hpp"


#include <string>
#include <vector>
#include <string_view>

enum class SystemMode
{
    Command,
    Storage
};


class DeviceControllerGPIO : public DeviceController
{
public:
    DeviceControllerGPIO() = default;

    void init() override;
    void update() override;

private:
    void processConfigCommand(std::string_view commandLine);
    void processHostCommand(std::string_view commandLine);
    
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
    SDCardHandler _sdCard;

    bool _lastStrobeState = true;
    bool _lastEjectState = true;
};