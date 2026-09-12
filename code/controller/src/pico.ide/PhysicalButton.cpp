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

    Event eventToReturn = Event::None;

    // single click detection
    if (!_pressed && _waitingForDoubleClick && (now - _releaseTime > kDoubleClickWindow_ms))
    {
        _waitingForDoubleClick = false;
        return Event::ShortPress;
    }

    if (currentPressed && !_pressed)
    {
        // falling edge detected
        if (now - _releaseTime > kDebounceTime_ms) 
        {
            _pressed = true;
            _pressTime = now;
            _longPressTriggered = false;
        }
    }
    else if (currentPressed && _pressed)
    {
        // button is still pressed, check for long press
        if (!_longPressTriggered && (now - _pressTime >= kLongPress_ms))
        {
            _longPressTriggered = true;
            _waitingForDoubleClick = false; // reset double click waiting state on long press
            eventToReturn = Event::LongPress;
        }
    }
    else if (!currentPressed && _pressed)
    {
        // rising edge detected (release)
        _pressed = false;        
        if (!_longPressTriggered && (now - _pressTime > kDebounceTime_ms))
        {
            if (_waitingForDoubleClick)
            {
                // Second release within the double click window, trigger double click event
                _waitingForDoubleClick = false;
                eventToReturn = Event::DoublePress;
            }
            else
            {
                // First release, start waiting for a potential double click
                _waitingForDoubleClick = true;
                _releaseTime = now;
            }
        }
    }
    
    return eventToReturn;
}