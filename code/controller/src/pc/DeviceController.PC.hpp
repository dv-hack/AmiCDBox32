// DeviceController.hpp
//
#pragma once

#include "DeviceController.hpp"
#include "CdDrive.hpp"

class DeviceControllerPC : public DeviceController
{
public:
    void init() override;
    void update() override;
    void updateLoop() override;

private:
    void cmd_mount( const std::string &);
    void cmd_unmount();
    void cmd_read();
    void cmd_showContents();


private:
    CdDrive _drive;
};