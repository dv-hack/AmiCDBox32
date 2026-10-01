// DeviceControllerGPIO.hpp
//
#pragma once

#include "DeviceController.hpp"

#include "CdDrive.hpp"
#include "DiscPlaylist.hpp"
#include "PhysicalButton.hpp"
#include "AudioPlayer.hpp"
#include "IDEController.hpp"
#include "Pins.hpp"

class DeviceControllerGPIO : public DeviceController
{
public:
    void init() override;
    void update() override;

    CdDrive &drive() { return _drive; }

private:
    CdDrive _drive;
    
    AudioPlayer *_audioPlayer{nullptr};

    IDEController _ideController;

    DiscPlaylist _playlist;
    
    //PhysicalButton _buttonControl{Pin::GP18};
};