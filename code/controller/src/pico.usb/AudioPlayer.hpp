// AudioPlayer.hpp
//
#include <cstdint>

class AudioPlayer
{
public:
    AudioPlayer(uint32_t dacPin) : _dacPin(dacPin) {}

    void startPlayback()
    {
        _isActive = true;
        // Hardware initialization...
    }

private:
    uint32_t _dacPin;
    bool _isActive = false;
};