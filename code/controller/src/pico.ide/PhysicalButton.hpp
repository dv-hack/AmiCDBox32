// PhysicalButton.hpp
//
#pragma once

#include <cstdint>

class PhysicalButton
{
public:
    enum class Event
    {
        None,
        ShortPress,
        DoublePress,
        LongPress
    };


public:
    PhysicalButton(uint8_t pin);
    Event update();

private:
    uint8_t  _pin;
    uint32_t _pressTime = 0;
    uint32_t _releaseTime = 0;
    
    bool _pressed = false;
    bool _longPressTriggered = false;
    bool _waitingForDoubleClick = false;
    
    static constexpr uint32_t kLongPress_ms = 1000;
    static constexpr uint32_t kDebounceTime_ms = 50;
    static constexpr uint32_t kDoubleClickWindow_ms = 300;
};