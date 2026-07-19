// PwmBuzzerPlayer.cpp
//
#include "PwmBuzzerPlayer.hpp"

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// ------------------------------------------------------------------------------------------------
PwmBuzzerPlayer::PwmBuzzerPlayer(uint32_t dacPin)
: _dacPin(dacPin) 
{}

// ------------------------------------------------------------------------------------------------
void PwmBuzzerPlayer::startPlayback()
{
    _isActive = true;
    
    gpio_set_function(_dacPin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(_dacPin);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // max clock
    pwm_config_set_wrap(&config, 255);    // 8-bit resolution (0-255)
    
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(_dacPin, 128);     // mid-signal (silence)
}

// ------------------------------------------------------------------------------------------------
void PwmBuzzerPlayer::stopPlayback()
{
    _isActive = false;
    uint slice_num = pwm_gpio_to_slice_num(_dacPin);
    pwm_set_enabled(slice_num, false);
    gpio_set_function(_dacPin, GPIO_FUNC_SIO);
    gpio_set_dir(_dacPin, GPIO_OUT);
    gpio_put(_dacPin, 0); // avoid noises when idle
}

// ------------------------------------------------------------------------------------------------
void PwmBuzzerPlayer::feedSector(const Sector& sector)
{
    if (!_isActive)
        return;

    // Un settore CD-DA = 2352 bytes. La struttura per ogni frame (4 bytes) è:
    // [Left LSB, Left MSB, Right LSB, Right MSB]
    
    for (int i = 0; i < kSectorSize; i += 4)
    {
        // 1. Ricostruzione 16-bit signed (Little Endian)
        const int16_t left = static_cast<int16_t>(sector.bytes[i] | (sector.bytes[i + 1] << 8));
        const int16_t right = static_cast<int16_t>(sector.bytes[i + 2] | (sector.bytes[i + 3] << 8));

        // 2. Downmix to Mono
        const int32_t mono = (left + right) / 2;

        // 3. Convert to 8-bit unsigned
        // Shift 8bit right and center at the middle of the sample
        const uint8_t sample8 = static_cast<uint8_t>((mono >> 8) + 128);

        pwm_set_gpio_level(_dacPin, sample8);

        // 4. simulate 44.1Hz timing with a sleep (~22.6 us per sample). (jittered sound output)
        sleep_us(22); 
    }
}