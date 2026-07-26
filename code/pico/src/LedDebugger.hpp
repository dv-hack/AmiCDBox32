// LedDebugger.hpp
//
#pragma once

#include <cstdint>

// extern "C" {
// void DebugLeds_Init();
// void DebugLed_Activity(bool state);
// void DebugLed_Error(bool state);
// void DebugLed_Ready(bool state);
// }

class LedDebugger
{
public:
    static LedDebugger &instance()
    {
        static LedDebugger singleton;
        return singleton;
    }

    void init();
    void signalActivity( bool state );
    void signalReady( bool state );
    void signalError( bool state );

private:
    LedDebugger() = default;
private:
    bool _initialized{false};

};

inline void setErrorLed( bool state ) { LedDebugger::instance().signalError(state); }
inline void setReadyLed( bool state ) { LedDebugger::instance().signalReady(state); }
inline void setActivityLed( bool state ) { LedDebugger::instance().signalActivity(state); }