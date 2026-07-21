// PwmBuzzerPlayer.hpp
//
#pragma once

#include "IAudioPlayer.hpp"
#include "Sector.hpp"


class PwmBuzzerPlayer : public IAudioPlayer
{
public:
    explicit PwmBuzzerPlayer(uint32_t dacPin);

    void startPlayback() override;
    void stopPlayback() override;
    void feedSector(const Sector &s) override;

private:
    uint32_t _dacPin;
    bool _isActive = false;

    int _dmaChannel = -1;
    int _currentBuffer = 0;

    uint32_t _sampleBuffer[2][588];
};