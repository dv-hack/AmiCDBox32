// main.pc.cpp
//
#include "Version.hpp"
#include "DeviceController.PC.hpp"


#include <iostream>


int main( int argc, char **argv)
{
    std::cout << "============================================================\n";
    std::cout << "============================================================\n";
    std::cout << "   AmiCDBox32 Debug Console [ VERSION " << FirmwareVersion << " ]\n";
    std::cout << "============================================================\n";
    std::cout << "============================================================\n";


    DeviceController *device = new DeviceControllerPC;

    device->init();
    device->updateLoop();

    delete device;
}