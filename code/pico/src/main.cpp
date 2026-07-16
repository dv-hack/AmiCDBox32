// main.cpp
//
#include "DeviceController.hpp"

#include "pico/stdlib.h"

int main()
{
    stdio_init_all();
    sleep_ms(2000); // Allow USB serial to hook up

    DeviceController controller;
    controller.init();

    while (true)
    {
        controller.update();
    }
}