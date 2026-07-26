// LedDebugger.cpp
//
#include "LedDebugger.hpp"

#include "pico/stdlib.h"

constexpr uint LED_READY_PIN = 11;
constexpr uint LED_ACT_PIN = 12;
constexpr uint LED_ERR_PIN = 13;

// ------------------------------------------------------------------------------------------------
void LedDebugger::init()
{
    if( _initialized )
        return;

    gpio_init(LED_READY_PIN); gpio_set_dir(LED_READY_PIN, GPIO_OUT);
    gpio_init(LED_ACT_PIN);   gpio_set_dir(LED_ACT_PIN, GPIO_OUT);
    gpio_init(LED_ERR_PIN);   gpio_set_dir(LED_ERR_PIN, GPIO_OUT);
    
    // turn off everything
    gpio_put(LED_READY_PIN, 0);
    gpio_put(LED_ACT_PIN, 0);
    gpio_put(LED_ERR_PIN, 0);

    _initialized = true;
}

// ------------------------------------------------------------------------------------------------
void LedDebugger::signalActivity( bool state ) { gpio_put(LED_ACT_PIN, state);   }
void LedDebugger::signalReady( bool state )    { gpio_put(LED_READY_PIN, state); }
void LedDebugger::signalError( bool state )    { gpio_put(LED_ERR_PIN, state);   }