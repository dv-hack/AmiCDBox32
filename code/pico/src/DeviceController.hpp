// DeviceController.hpp
//
#pragma once

class DeviceController
{
public:
    virtual ~DeviceController() {}
    virtual void init() = 0;
    virtual void update() = 0;

    virtual void updateLoop()
    {
        while(true) update(); 
    }
};
