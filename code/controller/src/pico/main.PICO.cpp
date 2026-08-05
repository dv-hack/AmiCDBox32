// main.cpp
//

#define TINY_USB_MODE
#if defined TINY_USB_MODE
#include "DeviceControllerUSB.hpp"
#else
#include "DeviceControllerGPIO.hpp"
#endif

#include "LedDebugger.hpp"


#define TINYUSB_MODE

int main()
{
    
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