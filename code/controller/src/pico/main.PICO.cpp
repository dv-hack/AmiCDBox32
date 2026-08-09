// main.cpp
//

#define TINY_USB_MODE
#if defined TINY_USB_MODE
#include "DeviceControllerUSB.hpp"
#else
#include "DeviceControllerGPIO.hpp"
#endif

#include "LedDebugger.hpp"
#include "Version.hpp"

#include <iostream>

#define TINYUSB_MODE

int main()
{
    std::cout << "============================================================\n";
    std::cout << "============================================================\n";
    std::cout << "   AmiCDBox32 Debug Console [ VERSION " << FirmwareVersion << " ]\n";
    std::cout << "============================================================\n";
    std::cout << "============================================================\n";

#if defined TINY_USB_MODE
    DeviceController* controller = new DeviceControllerUSB;
#else
    DeviceController* controller = new DeviceControllerGPIO;
#endif

    LedDebugger::instance().init();


    controller->init();
    controller->updateLoop();
    
    
    delete controller;
}