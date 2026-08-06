// PwmBuzzerPlayer.hpp
//
#pragma once

#include "IAudioPlayer.hpp"

class PwmBuzzerPlayer : public IAudioPlayer
{
public:
    explicit PwmBuzzerPlayer(uint32_t dacPin);

    void startPlayback( Disc *disc, uint32_t startLBA, uint32_t endLBA ) override;
    void stopPlayback() override;
    bool playFrame() override;

    uint32_t currentLBA() const override;
    bool isPlaying() const override;

private:
    void play(const uint8_t *sectorData);

private:
    Disc *_disc{nullptr}; // not owned
    
    uint32_t _dacPin{0};
    uint32_t _currentAudioLba{0};
    uint32_t _targetAudioLba{0};

    
    bool _isActive = false;
    int _dmaChannel = -1;
    int _currentBuffer = 0;

    uint32_t _sampleBuffer[2][588];
};