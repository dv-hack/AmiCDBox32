// main.cpp
//
#include "DeviceControllerGPIO.hpp"

int main()
{
    DeviceController* controller = new DeviceControllerGPIO;

    controller->init();
    controller->updateLoop();

    delete controller;
}