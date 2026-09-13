// DeviceControllerGPIO.cpp
//
#include "DeviceControllerGPIO.hpp"

#include "Version.hpp"
#include "Disc.hpp"
#include "Mounter.hpp"
#include "VirtualNavDisc.hpp"

#include "bsp/board.h"

#include <iostream>

// ------------------------------------------------------------------------------------------------
void DeviceControllerGPIO::init()
{
    stdio_init_all();
    sleep_ms(15000); // Allow USB serial to hook up

    std::cout << "============================================================\n";
    std::cout << "============================================================\n";
    std::cout << "   AmiCDBox32  Console [ VERSION " << FirmwareVersion << " ]\n";
    std::cout << "============================================================\n";
    std::cout << "============================================================\n";


    //board_init();

    FileSystem::init();
    _ideController.init();

    // @todo: audio board init

    _drive.insertDisc( new VirtualNavDisc( FileSystem::Path("0:/"), "SD_NAVIGATOR" ) );
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerGPIO::update()
{
    const auto btnEvent = _buttonControl.update();
    
    switch (btnEvent)
    {
        case PhysicalButton::Event::ShortPress:
            std::cout << "[SYSTEM] Single press: Next track/ISO...\n";
            break;
            
        case PhysicalButton::Event::DoublePress:
            std::cout << "[SYSTEM] Double press: Mounting selected image...\n";
            break;

        case PhysicalButton::Event::LongPress:
            if (_drive.state() == CdDrive::State::Ready)
            {
                std::cout << "[SYSTEM] Eject long press: unmounting disc...\n";
                auto d = _drive.eject();
                delete d;
            }
            else
            {
                std::cout << "[SYSTEM] Eject long press: Switch to VirtualNavDisc...\n";
                _drive.insertDisc(new VirtualNavDisc(FileSystem::Path("0:/"), "SD_NAVIGATOR"));
            }
            break;

        case PhysicalButton::Event::None:
        default:
            break;
    }

    _ideController.update();
}