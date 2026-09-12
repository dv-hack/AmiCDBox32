// PhysicalButton.cpp
//
#include "PhysicalButton.hpp"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// ------------------------------------------------------------------------------------------------
PhysicalButton::PhysicalButton(uint8_t pin)
    : _pin(pin)
{
    gpio_init(_pin);
    gpio_set_dir(_pin, GPIO_IN);
    gpio_pull_up(_pin);
}

// ------------------------------------------------------------------------------------------------
PhysicalButton::Event PhysicalButton::update()
{
    bool currentPressed = !gpio_get(_pin); // Active Low
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    if (currentPressed && !_pressed)
    {
        _pressed = true;
        _pressTime = now;
        _longPressTriggered = false;
    }
    else if (currentPressed && _pressed)
    {
        if (!_longPressTriggered && (now - _pressTime >= kLongPress_ms))
        {
            _longPressTriggered = true;
            return Event::LongPress;
        }
    }
    else if (!currentPressed && _pressed)
    {
        _pressed = false;        
        if (!_longPressTriggered && (now - _pressTime > kDebounceTime_ms))
            return Event::ShortPress;
    }
    return Event::None;
}