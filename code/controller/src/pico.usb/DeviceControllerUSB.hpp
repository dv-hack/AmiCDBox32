// DeviceControllerUSB.hpp
//
#pragma once

#include "DeviceController.hpp"

#include "CdDrive.hpp"
#include "DiscPlaylist.hpp"
#include "PhysicalButton.hpp"
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

    DiscPlaylist _playlist;
    
    PwmBuzzerPlayer _pwmBuzzerPlayer{20};
    PhysicalButton _buttonMount{15};
    PhysicalButton _buttonNext {14};
};