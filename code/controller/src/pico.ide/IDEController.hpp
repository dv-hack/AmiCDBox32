// IDEController.hpp
//
#pragma once

#include "Pins.hpp"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include <cstdint>

class DeviceControllerGPIO;

class IDEController
{
public:
    // enum class BusOperation
    // {
    //     None,
    //     RegisterRead,
    //     RegisterWrite,
    //     DataBlockRead,
    //     DataBlockWrite
    // };

    struct IDETransaction
    {
        uint8_t  address;   // Indirizzo registro (A0..A2 + CS0/CS1)
        bool     isWrite;   // true = DIOW#, false = DIOR#
        uint16_t data;      // Dato a 16 bit letto/da scrivere
    };

public:
    IDEController();
    ~IDEController();

    void init(PIO pioInstance = pio0);
    void update();

    // Gestione direzione transceiver 74LVC245
    void setTransceiverDirection(bool driveToAmiga);
    void enableTransceivers(bool enableLow, bool enableHigh);

private:
    static constexpr uint32_t DATA_BUS_MASK = 0x000000FF; // GP00..GP07 (Bit 0..7)

    // Gestione diretta dei GPIO dati e dei transceiver
    void setBusDataDirection(bool driveToHost);
    void writeDataByte(uint8_t byte);
    
    uint8_t readDataByte();
    static void pioInterruptHandler();
    void processTransaction(const IDETransaction& tx);

private:
    PIO      _pio{pio0};
    uint     _smR{0};          // State Machine per la Lettura (DIOR)
    uint     _smW{0};          // State Machine per la Scrittura (DIOW)
    uint     _offsetR{0};
    uint     _offsetW{0};

    // Registro di stato ATA (Task File Registers)
    uint8_t  _statusRegister{0x00};
    uint8_t  _errorRegister{0x00};

    static IDEController* s_instance;
};