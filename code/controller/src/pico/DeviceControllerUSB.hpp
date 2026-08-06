// DeviceControllerUSB.hpp
//
#pragma once

#include "DeviceController.hpp"

#include "CdDrive.hpp"
#include "PwmBuzzerPlayer.hpp"

class DeviceControllerUSB : public DeviceController
{
public:
    DeviceControllerUSB();
    void init() override;
    void update() override;

    CdDrive &drive() { return _drive; }

private:
    CdDrive _drive;
    PwmBuzzerPlayer _pwmBuzzerPlayer{20};
};