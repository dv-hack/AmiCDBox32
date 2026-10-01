// IDEController.hpp
//
#pragma once

#include "Pins.hpp"
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include <cstdint>
#include <array>

class IDEController
{
public:
    struct IDETransaction
    {
        uint8_t  address;   
        bool     isWrite;   
        uint16_t data;      
        uint32_t rawSample;
    };

public:
    IDEController();
    ~IDEController();

    void init(PIO pioInstance = pio0);
    void update();
    void printTrace();

private:
    static void pioInterruptHandler();
    
    void processTransaction(const IDETransaction& tx);

    void handleCommand(uint8_t command);
    void handleSCSIPacket();
    void resetDevice();
    //void setIORDY(bool ready);

    uint16_t readRegister16(uint8_t address);
    void     writeRegister16(uint8_t address, uint16_t value);
    void     prepareIdentifyPacket();

private:
    PIO      _pio{pio0};
    uint     _smR{0};          
    uint     _smW{0};          
    uint     _offsetR{0};
    uint     _offsetW{0};

    std::array<uint8_t, 8> _taskFile{};
    uint8_t _deviceControl = 0;
    uint8_t _status = 0x50;
    uint8_t _error = 0;

    std::array<uint8_t, 12> _packet{};
    uint8_t _packetIndex = 0;
    uint8_t _packetExpected = 0;

    std::array<uint8_t, 2048> _dataBuffer{};
    uint32_t _dataLength = 0;
    uint32_t _dataIndex = 0;

    bool _packetPhase = false;
    bool _dataInPhase = false;
    bool _dataOutPhase = false;

    struct TraceEvent {
        uint8_t address;
        bool isWrite;
        uint16_t data;
        uint32_t rawSample;
        uint32_t gpioSnapshot;        
    };
    
    static constexpr size_t TRACE_BUFFER_SIZE = 256;
    TraceEvent _traceBuffer[TRACE_BUFFER_SIZE];
    volatile size_t _traceHead{0};
    size_t _traceTail{0};    

    static IDEController* s_instance;
};