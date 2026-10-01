// IDEController.cpp
//
#include "IDEController.hpp"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "ide_bus.pio.h"
#include <iostream>

static constexpr uint32_t DATA_BUS_MASK = 0x000000FF; // GP00..GP07
static constexpr uint32_t STROBE_TIMEOUT_CYCLES = 2000000; //20000; // Timeout di sicurezza per i cicli ISR

namespace AtaRegister
{
    constexpr uint8_t Data          = 0x00;
    constexpr uint8_t Error         = 0x01;
    constexpr uint8_t Features      = 0x01;
    constexpr uint8_t SectorCount   = 0x02;
    constexpr uint8_t SectorNumber  = 0x03;
    constexpr uint8_t CylinderLow   = 0x04;
    constexpr uint8_t CylinderHigh  = 0x05;
    constexpr uint8_t DriveHead     = 0x06;
    constexpr uint8_t Status        = 0x07;
    constexpr uint8_t Command       = 0x07;
    constexpr uint8_t AlternateStatus = 0x0E;
    constexpr uint8_t DeviceControl   = 0x0E;
    constexpr uint8_t DriveAddress    = 0x0F;
}

IDEController* IDEController::s_instance = nullptr;

IDEController::IDEController() { s_instance = this; }

IDEController::~IDEController()
{
    if (_pio)
    {
        const uint irqNum = (_pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0;
        irq_set_enabled(irqNum, false);
        pio_set_irq0_source_enabled(_pio, pis_interrupt0, false);
    
        pio_sm_set_enabled(_pio, _smR, false);
        pio_sm_unclaim(_pio, _smR);
        pio_remove_program(_pio, &ide_strobe_dior_program, _offsetR);
    
        pio_sm_set_enabled(_pio, _smW, false);
        pio_sm_unclaim(_pio, _smW);
        pio_remove_program(_pio, &ide_strobe_diow_program, _offsetW);
    }
    s_instance = nullptr;
}

void IDEController::init(PIO pioInstance)
{
    _pio = pioInstance;

    std::cout << "[IDEController] Initializing..." << std::endl;


    // Bus Dati (GP00..GP07)
    for (int pin = Pin::GP00; pin <= Pin::GP07; ++pin)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // Configurazione Pin Controllo Multiplexer (GP08..GP12)
    gpio_init(Pin::GP08); gpio_set_dir(Pin::GP08, GPIO_OUT); gpio_put(Pin::GP08, 0); // DIR (0 = Amiga->Pico)
    gpio_init(Pin::GP09); gpio_set_dir(Pin::GP09, GPIO_OUT); gpio_put(Pin::GP09, 1); // U1 OE# HIGH (Disabilitato)
    gpio_init(Pin::GP10); gpio_set_dir(Pin::GP10, GPIO_OUT); gpio_put(Pin::GP10, 1); // U2 OE# HIGH (Disabilitato)
    gpio_init(Pin::GP11); gpio_set_dir(Pin::GP11, GPIO_OUT); gpio_put(Pin::GP11, 0); // U3 LE LOW (Trasparente off)
    gpio_init(Pin::GP12); gpio_set_dir(Pin::GP12, GPIO_OUT); gpio_put(Pin::GP12, 1); // U3 OE# HIGH (Disabilitato)

    // Configurazione Pin Indirizzo e Strobe (GP13..GP20)
    for (int pin = Pin::GP13; pin <= Pin::GP20; ++pin)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

// Inizializzazione fisica del pin IORDY per pilotare OE# del 74LVC573
    gpio_init(Pin::GP21);
    gpio_set_dir(Pin::GP21, GPIO_OUT);
    gpio_put(Pin::GP21, 1); // GP21=HIGH -> OE# inattivo -> Uscita High-Z (Host libero)
    pio_gpio_init(_pio, Pin::GP21); // Cede il controllo logico al PIO

    // Configurazione _smR
    _offsetR = pio_add_program(_pio, &ide_strobe_dior_program);
    _smR = pio_claim_unused_sm(_pio, true);
    pio_sm_config cR = ide_strobe_dior_program_get_default_config(_offsetR);
    sm_config_set_in_pins(&cR, Pin::GP15);
    sm_config_set_in_shift(&cR, false, false, 32); 
    sm_config_set_set_pins(&cR, Pin::GP21, 1); 
    sm_config_set_jmp_pin(&cR, Pin::GP16); // <-- CONFIGURA GP16 (CS1#) COME JUMP PIN PER _smR
    pio_sm_init(_pio, _smR, _offsetR, &cR);

    // Configurazione _smW
    _offsetW = pio_add_program(_pio, &ide_strobe_diow_program);
    _smW = pio_claim_unused_sm(_pio, true);
    pio_sm_config cW = ide_strobe_diow_program_get_default_config(_offsetW);
    sm_config_set_in_pins(&cW, Pin::GP15);
    sm_config_set_in_shift(&cW, false, false, 32);
    sm_config_set_set_pins(&cW, Pin::GP21, 1); 
    sm_config_set_jmp_pin(&cW, Pin::GP16); // <-- CONFIGURA GP16 (CS1#) COME JUMP PIN PER _smW
    pio_sm_init(_pio, _smW, _offsetW, &cW); 
    
    // Svuota le FIFO del PIO per scartare campionamenti spuri prima dell'avvio
    pio_sm_clear_fifos(_pio, _smR);
    pio_sm_clear_fifos(_pio, _smW);

    // Abilita le State Machine
    pio_sm_set_enabled(_pio, _smR, true);
    pio_sm_set_enabled(_pio, _smW, true);

    // Pulisce interrupt pendenti nel PIO
    pio_interrupt_clear(_pio, 0);

    // Abilita l'interrupt lato CPU
    const uint irqNum = (_pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0;
    //pio_set_irq0_source_enabled(_pio, pis_interrupt0, true);
    pio_set_irq0_source_enabled(_pio, pis_sm0_rx_fifo_not_empty, true);
    pio_set_irq0_source_enabled(_pio, pis_sm1_rx_fifo_not_empty, true);    
    irq_set_exclusive_handler(irqNum, IDEController::pioInterruptHandler);
    irq_set_enabled(irqNum, true);

    std::cout << "IDEController initialized (3-Chip Multiplexed)" << std::endl;
    resetDevice();
}

// void IDEController::setIORDY(bool ready)
// {
//     gpio_put(Pin::GP21, ready ? 1 : 0);

//     // if (ready) {
//     //     // Disabilita l'uscita: lascia che la resistenza dell'host porti il pin a 5V
//     //     gpio_set_dir(Pin::GP21, GPIO_IN); 
//     // } else {
//     //     // Abilita l'uscita e forza a GND per fermare l'host
//     //     gpio_put(Pin::GP21, 0);
//     //     gpio_set_dir(Pin::GP21, GPIO_OUT); 
//     // }    
// }

void IDEController::pioInterruptHandler()
{
    if (!s_instance) return;
    pio_interrupt_clear(s_instance->_pio, 0);

    auto processRawSample = [](uint32_t rawSample, bool isWrite)
    {
        const bool cs0_pin = (rawSample & (1u << 0)); 
        const bool cs1_pin = (rawSample & (1u << 1)); 
        const bool a0_pin  = (rawSample & (1u << 2)); 
        const bool a1_pin  = (rawSample & (1u << 3)); 
        const bool a2_pin  = (rawSample & (1u << 4)); 

        const bool cs0_active = !cs0_pin;
        const bool cs1_active = !cs1_pin;
        const uint8_t base_addr = (a2_pin << 2) | (a1_pin << 1) | a0_pin;

        IDETransaction tx{};
        tx.isWrite = isWrite;
        tx.rawSample = rawSample;
        tx.address = cs0_active ? base_addr : (cs1_active ? (0x08 | base_addr) : 0xFF);
        return tx;
    };

    while (!pio_sm_is_rx_fifo_empty(s_instance->_pio, s_instance->_smR))
    {
        const uint32_t rawSample = pio_sm_get(s_instance->_pio, s_instance->_smR);
        const auto tx = processRawSample(rawSample, false);
        
        if (tx.address != 0xFF) {
            s_instance->processTransaction(tx);
        } else {
            // CICLO FANTASMA: Sblocca immediatamente il PIO altrimenti IORDY resta basso per sempre!
            pio_sm_put(s_instance->_pio, s_instance->_smR, 0);
        }
    }

    while (!pio_sm_is_rx_fifo_empty(s_instance->_pio, s_instance->_smW))
    {
        const uint32_t rawSample = pio_sm_get(s_instance->_pio, s_instance->_smW);
        const auto tx = processRawSample(rawSample, true);
        
        if (tx.address != 0xFF) {
            s_instance->processTransaction(tx);
        } else {
            // CICLO FANTASMA: Sblocca immediatamente il PIO
            pio_sm_put(s_instance->_pio, s_instance->_smW, 0);
        }
    }
}

void IDEController::processTransaction(const IDETransaction& tx)
{
    uint16_t traceData = 0;
    const bool is16Bit = (tx.address == AtaRegister::Data);
    bool stallTimeout = false;
    uint32_t stallGpioSnapshot = 0;

    if (tx.isWrite)
    {
        // HOST SCRIVE (Amiga -> Pico)
        gpio_set_dir_masked(DATA_BUS_MASK, 0); // Pico in ingresso
        gpio_put(Pin::GP08, 0); // DIR = 0 (Amiga -> Pico)

        uint16_t receivedData = 0;

        if (is16Bit)
        {
            // Legge il byte alto da U2
            gpio_put(Pin::GP10, 0); // Attiva U2 OE#
            //tight_loop_contents();
            busy_wait_us_32(1);
            receivedData = (gpio_get_all() & DATA_BUS_MASK) << 8;
            gpio_put(Pin::GP10, 1); // Disattiva U2

            // Legge il byte basso da U1
            gpio_put(Pin::GP09, 0); // Attiva U1 OE#
            //tight_loop_contents();
            busy_wait_us_32(1);
            receivedData |= (gpio_get_all() & DATA_BUS_MASK);
            gpio_put(Pin::GP09, 1); // Disattiva U1
            // std::cout << "[DBG-DATA] bus=0x" << std::hex << (receivedData)
            //       << " addr=0x" << (int)tx.address << std::dec << std::endl;

        }
        else
        {
            // Lettura a 8-bit (solo U1)
            gpio_put(Pin::GP09, 0);
            //tight_loop_contents();
            busy_wait_us_32(1);
            receivedData = gpio_get_all() & DATA_BUS_MASK;
            // std::cout << "[DBG-DATA] bus=0x" << std::hex << (receivedData)
            //       << " addr=0x" << (int)tx.address << std::dec << std::endl;

            gpio_put(Pin::GP09, 1);
        }

        writeRegister16(tx.address, receivedData);
        traceData = receivedData;

        // CROSSTALK PREVENTION: Se _smR è scattata per errore, svuotala e sbloccala ORA
        // if (!pio_sm_is_rx_fifo_empty(s_instance->_pio, _smR)) {
        //     pio_sm_get(s_instance->_pio, _smR);    // Scarta la lettura spuria
        //     pio_sm_put(s_instance->_pio, _smR, 0); // Sblocca _smR in modo che alzi GP21
        // }        

        // Sblocca l'host rilasciando IORDY (ritorna in alta impedenza)
        // Forza la State Machine a eseguire "set pindirs, 0" in 1 ciclo di clock
        pio_sm_put(s_instance->_pio, _smW, 0);
#if 1
        uint32_t timeout = 2000000; // ~16ms, perfetto per non affamare il Pico
        while (gpio_get(Pin::GP14) == 0 && --timeout > 0) {
            tight_loop_contents();
        }
        if (timeout == 0)
        {
            stallGpioSnapshot = gpio_get_all();
            stallTimeout = true;
        } 
#endif
    }
    else
    {
        // HOST LEGGE (Pico -> Amiga)
        const uint16_t valueToSend = readRegister16(tx.address);
        
        gpio_set_dir_masked(DATA_BUS_MASK, DATA_BUS_MASK); // Pico in uscita
        
        if (is16Bit)
        {
            gpio_put(Pin::GP12, 1); // U3 OE# disabilitato durante l'aggiornamento

            // 1. Prepara e latcha il byte alto in U3
            gpio_put_masked(DATA_BUS_MASK, (valueToSend >> 8) & 0xFF);
            gpio_put(Pin::GP11, 1); // U3 LE HIGH
            tight_loop_contents();
            gpio_put(Pin::GP11, 0); // U3 LE LOW (Lato Alto Latchato)
            gpio_put(Pin::GP12, 0); // U3 OE# LOW (Lato Alto Abilitato sul Bus)

            // 2. Prepara il byte basso in U1
            gpio_put_masked(DATA_BUS_MASK, valueToSend & 0xFF);
            gpio_put(Pin::GP08, 1); // DIR = 1 (Pico -> Amiga)
            gpio_put(Pin::GP09, 0); // U1 OE# LOW (Lato Basso Abilitato sul Bus)
        }
        else
        {
            // 8-bit mode: spinge solo la parte bassa su U1, U2/U3 restano in Hi-Z
            gpio_put_masked(DATA_BUS_MASK, valueToSend & 0xFF);
            gpio_put(Pin::GP08, 1); // DIR = 1 (Pico -> Amiga)
            gpio_put(Pin::GP09, 0); 
        }

        traceData = valueToSend;

        // // PREVENZIONE CROSSTALK: Se _smW è scattata per errore, svuotala e sbloccala ORA
        // if (!pio_sm_is_rx_fifo_empty(s_instance->_pio, _smW)) 
        // {
        //     pio_sm_get(s_instance->_pio, _smW);    // Scarta la scrittura spuria
        //     pio_sm_put(s_instance->_pio, _smW, 0); // Sblocca _smW in modo che alzi GP21
        // }        
        // Sblocca l'host rilasciando IORDY (ritorna in alta impedenza)
        // Forza la State Machine a eseguire "set pindirs, 0" in 1 ciclo di clock
        pio_sm_put(s_instance->_pio, _smR, 0);
#if 1
        uint32_t timeout = 2000000;
        while (gpio_get(Pin::GP13) == 0 && --timeout > 0) {
            tight_loop_contents();
        }
        if (timeout == 0)
        {
            stallGpioSnapshot = gpio_get_all();
            stallTimeout = true;
        } 
#endif

        busy_wait_us_32(1); // Attende un ciclo di clock per garantire che l'host legga i dati (data hold time t6)


        // Disabilita immediatamente i transceiver per liberare il bus IDE
        gpio_put(Pin::GP09, 1); // U1 OE# HIGH
        if (is16Bit)
            gpio_put(Pin::GP12, 1); // U3 OE# HIGH
        
        // Riporta il Pico in ingresso per prevenire contention locali
        gpio_set_dir_masked(DATA_BUS_MASK, 0);
    }

    size_t nextHead = (_traceHead + 1) % TRACE_BUFFER_SIZE;
    if (nextHead != _traceTail)
    {
        if (stallTimeout) 
        {
            // Salva il timeout usando 'data' per memorizzare l'indirizzo che ha fallito
            _traceBuffer[_traceHead] = { 0xFE, tx.isWrite, tx.address, tx.rawSample, stallGpioSnapshot };
            _traceHead = nextHead;
            nextHead = (_traceHead + 1) % TRACE_BUFFER_SIZE;
        }
        
        if (nextHead != _traceTail) 
        {
            // Registra sempre la transazione originale in chiaro
            _traceBuffer[_traceHead] = { tx.address, tx.isWrite, traceData, tx.rawSample, 0 };
            _traceHead = nextHead;
        }
    }
}

void IDEController::update() 
{
    printTrace(); 
    tight_loop_contents();
}

void IDEController::printTrace()
{
    while (_traceTail != _traceHead) 
    {
        TraceEvent e = _traceBuffer[_traceTail];
        _traceTail = (_traceTail + 1) % TRACE_BUFFER_SIZE;

        if (e.address == 0xFE) 
        {
            std::cout << "[DBG-STALL] timeout waiting "
                      << (e.isWrite ? "DIOW#" : "DIOR#")
                      << " release (Reg: 0x" << std::hex << static_cast<int>(e.data)
                      << " raw=0x" << e.rawSample
                      << " GP13=" << ((e.gpioSnapshot >> Pin::GP13) & 1u)
                      << " GP14=" << ((e.gpioSnapshot >> Pin::GP14) & 1u)
                      << " GP16=" << ((e.gpioSnapshot >> Pin::GP16) & 1u)
                      << " GP21=" << ((e.gpioSnapshot >> Pin::GP21) & 1u)
                      << std::dec << ")\n";
        } 
        else 
        {
            std::cout << (e.isWrite ? "[AMI >> PICO]" : "[AMI << PICO]") 
                      << " Reg: 0x" << std::hex << (int)e.address 
                      << " | Data: 0x" << (int)e.data << std::dec << "\n";
        }    
    }
}

uint16_t IDEController::readRegister16(uint8_t address)
{
    switch (address)
    {
        case AtaRegister::Data:
        {
            if (!_dataInPhase || _dataIndex >= _dataLength)
            {
                _dataInPhase = false;
                _status = 0x50; // DRDY
                return 0x0000;
            }

            const uint16_t value =
                static_cast<uint16_t>(_dataBuffer[_dataIndex]) |
                (static_cast<uint16_t>(_dataBuffer[_dataIndex + 1]) << 8);

            _dataIndex += 2;

            if (_dataIndex >= _dataLength)
            {
                _dataInPhase = false;
                _status = 0x50; // Fine trasferimento: DRQ va a 0, resta DRDY
            }

            return value;
        }        
#if 0
        case AtaRegister::Data:
        {
            if (!_dataInPhase || _dataIndex + 1 >= _dataLength)
            {
                _dataInPhase = false;
                _status = 0x50; // DRQ basso
                return 0;
            }

            const uint16_t value =
                static_cast<uint16_t>(_dataBuffer[_dataIndex]) |
                (static_cast<uint16_t>(_dataBuffer[_dataIndex + 1]) << 8);

            _dataIndex += 2;

            if (_dataIndex >= _dataLength)
            {
                _dataInPhase = false;
                _status = 0x50;
            }

            return value;
        }
#endif        
        case AtaRegister::Error: return _error;
        case AtaRegister::SectorCount:
        case AtaRegister::SectorNumber:
        case AtaRegister::CylinderLow:
        case AtaRegister::CylinderHigh:
        case AtaRegister::DriveHead:
            return _taskFile[address];

        case AtaRegister::Status: return _status;
        case AtaRegister::AlternateStatus: return _status;
        case AtaRegister::DriveAddress: return 0x00;

        default: return 0x00;
    }
}

void IDEController::writeRegister16(uint8_t address, uint16_t value)
{
    const uint8_t byteValue = static_cast<uint8_t>(value & 0xFF);

    switch (address)
    {
        case AtaRegister::Features:
            _taskFile[AtaRegister::Features] = byteValue;
            break;

        case AtaRegister::SectorCount:
        case AtaRegister::SectorNumber:
        case AtaRegister::CylinderLow:
        case AtaRegister::CylinderHigh:
            _taskFile[address] = byteValue;
            break;
        
        case AtaRegister::DriveHead:
            _taskFile[AtaRegister::DriveHead] = byteValue;
            // Bit 4 (0x10) indica il drive: 0 = Master (Noi), 1 = Slave
            if ((byteValue & 0x10) == 0) 
            {
                // Se viene selezionato il Master, assicuriamoci di essere PRONTI
                if (!_packetPhase && !_dataInPhase) {
                    _status = 0x50; // DRDY | DSC
                }
            } 
            else 
            {
                // Se l'host seleziona lo Slave (che non esiste), lo stato va a 0x00
                _status = 0x00; 
            }
            break;

        case AtaRegister::Command:
            handleCommand(byteValue);
            break;

        case AtaRegister::DeviceControl:
            _deviceControl = byteValue;
            if (byteValue & (1u << 2)) resetDevice();
            break;

        case AtaRegister::Data:
            if (_packetPhase && _packetIndex < _packetExpected)
            {
                _packet[_packetIndex++] = byteValue;
                _packet[_packetIndex++] = static_cast<uint8_t>(value >> 8);
                
                if (_packetIndex >= _packetExpected)
                {
                    _packetPhase = false;
                    handleSCSIPacket();
                }
            }
            break;

        default:
            break;
    }
}

void IDEController::resetDevice()
{
    _taskFile.fill(0);
    
    // Inietta la firma ATAPI (CD-ROM)
    _taskFile[AtaRegister::SectorCount]  = 0x01;
    _taskFile[AtaRegister::SectorNumber] = 0x01;
    _taskFile[AtaRegister::CylinderLow]  = 0x14;
    _taskFile[AtaRegister::CylinderHigh] = 0xEB;    
    
    _deviceControl = 0;
    _error = 0;
    _status = 0x50; // DRDY | DSC
    //_status = 0x00; // DRDY | DSC

    _packet.fill(0);
    _packetIndex = 0;
    _packetExpected = 0;
    _dataLength = 0;
    _dataIndex = 0;
    _packetPhase = false;
    _dataInPhase = false;
    _dataOutPhase = false;
}

void IDEController::handleCommand(uint8_t command)
{
    _taskFile[AtaRegister::Command] = command;
    _error = 0;

    switch (command)
    {
        case 0x08: // ATAPI SOFT RESET
            resetDevice(); 
            // Dopo il reset, il dispositivo deve asserire DRDY
            _status = 0x50; 
            break;

        case 0xA1: // IDENTIFY PACKET DEVICE
            _status = 0x50;
            prepareIdentifyPacket();
            break;
        case 0xA0: // PACKET
            _packetIndex = 0;
            _packetExpected = 12;
            _packetPhase = true;
            _taskFile[AtaRegister::SectorCount] = 1; 
            _status = 0x58; // DRDY | DSC | DRQ
            break;
        case 0xEC: // IDENTIFY DEVICE
            _error = 0x04;
            _status = 0x51; 
            break;
        default:
            _error = 0x04; // ABRT
            _status = 0x51;
            break;
    }

    //setIORDY(true); // Segnala all'host che il device è pronto per la lettura/scrittura
}

void IDEController::prepareIdentifyPacket()
{
    _dataBuffer.fill(0); 
    _dataIndex = 0; 
    _dataLength = 512; 
 
    auto writeWord = [this](size_t wordIndex, uint16_t value) 
    { 
        const size_t offset = wordIndex * 2; 
        _dataBuffer[offset]     = static_cast<uint8_t>(value & 0xFF); 
        _dataBuffer[offset + 1] = static_cast<uint8_t>(value >> 8); 
    };

    auto writeAtaString = [this](
                              size_t firstWord,
                              size_t wordCount,
                              std::string_view text)
    {
        const size_t offset = firstWord * 2;
        const size_t length = wordCount * 2;

        for (size_t i = 0; i < length; i += 2)
        {
            const char first = i < text.size() ? text[i] : ' ';
            const char second = i + 1 < text.size() ? text[i + 1] : ' ';

            _dataBuffer[offset + i] = static_cast<uint8_t>(second);
            _dataBuffer[offset + i + 1] = static_cast<uint8_t>(first);
        }
    };

    // Word 0: 
    // ATAPI, device removable, packet command di 12 byte. 
    writeWord(0, 0x85C0); 
 
    // Word 1: numero cilindri non significativo per ATAPI. 
    writeWord(1, 0); 
 
    // Word 49: capabilities. 
    // Bit 9 = LBA support. 
    writeWord(49, 0x0200); 
 
    // Word 50: reserved. 
    writeWord(50, 0); 
 
    // Word 52: obsolete. 
    writeWord(52, 0); 
 
    // Word 53: valid words. 
    writeWord(53, 0x0007); 
 
    // Word 63: PIO/DMA support. 
    writeWord(63, 0x0007); 
 
    // Word 64: PIO timing mode. 
    writeWord(64, 0x0003); 
 
    // Word 65: minimum multiword DMA cycle time. 
    writeWord(65, 120); 
 
    // Word 66: recommended multiword DMA cycle time. 
    writeWord(66, 120); 
 
    // Word 67: minimum PIO cycle time. 
    writeWord(67, 120); 
 
    // Word 68: minimum PIO cycle time without IORDY. 
    writeWord(68, 120); 
 
    // Word 80: ATA/ATAPI major version. 
    writeWord(80, 0x003E); 
 
    // Word 81: minor version. 
    writeWord(81, 0x0010); 
 
    // Word 82: supported command sets. 
    writeWord(82, 0); 
 
    // Word 83: supported command sets. 
    writeWord(83, 0); 
 
    // Word 84: supported command sets extension. 
    writeWord(84, 0); 
 
    // Word 85..87: enabled command sets. 
    writeWord(85, 0); 
    writeWord(86, 0); 
    writeWord(87, 0); 
 
    // Word 88: Ultra DMA modes. 
    writeWord(88, 0); 
 
    // Word 126: removable media status notification. 
    writeWord(126, 0); 
 
    // Word 127: security status. 
    writeWord(127, 0); 
 
    // Word 128: security. 
    writeWord(128, 0); 
 
    // Word 136: media rotation rate. 
    writeWord(136, 1); 
 
    // Word 160: packet size. 
    // Bit 0 = 12-byte packet. 
    writeWord(160, 0x0001); 
 
    // Word 164: BCA support. 
    writeWord(164, 0); 
 
    // Word 165: maximum queue depth. 
    writeWord(165, 0); 
 
    // Word 166: reserved. 
    writeWord(166, 0); 
 
    // Word 126..255 rimangono a zero. 
 
    // ATAPI model string: 40 caratteri, words 27..46. 
    writeAtaString(27, 20, "AmiCDBox32 ATAPI CD-ROM"); 
 
    // Serial number: 20 caratteri, words 10..19. 
    writeAtaString(10, 10, "AMICDBOX32"); 
 
    // Firmware revision: 8 caratteri, words 23..26. 
    writeAtaString(23, 4, "0.5.0"); 
 
    _dataInPhase = true; 
    _dataOutPhase = false; 
    _status = 0x58; // DRDY | DSC | DRQ 
}

void IDEController::handleSCSIPacket()
{
    const uint8_t opcode = _packet[0];
    _error = 0;

    switch (opcode)
    {
        case 0x00: // TEST UNIT READY
        {
            // Il drive virtuale è pronto e ha un disco inserito
            _status = 0x50; // DRDY | DSC (DRQ disattivato)
            break;
        }

        case 0x12: // INQUIRY (Richiesta informazioni identificative drive)
        {
            _dataBuffer.fill(0);
            _dataBuffer[0] = 0x05; // Periferica CD-ROM / DVD
            _dataBuffer[1] = 0x80; // Supporto Rimovibile
            _dataBuffer[2] = 0x02; // Conformità ANSI SCSI-2
            _dataBuffer[3] = 0x02; // Formato risposta
            _dataBuffer[4] = 31;   // Lunghezza dati aggiuntivi (36 - 5)

            // Vendor ID (8 char), Product ID (16 char), Revision (4 char)
            std::string_view vendor  = "AmiCDBox";
            std::string_view product = "Virtual CD-ROM  ";
            std::string_view rev     = "0.50";

            std::copy(vendor.begin(), vendor.end(), _dataBuffer.begin() + 8);
            std::copy(product.begin(), product.end(), _dataBuffer.begin() + 16);
            std::copy(rev.begin(), rev.end(), _dataBuffer.begin() + 32);

            _dataIndex = 0;
            _dataLength = 36;
            _dataInPhase = true;
            _status = 0x58; // DRDY | DSC | DRQ (Pronto all'invio dei dati)
            break;
        }

        case 0x25: // READ CAPACITY (Dimensione e blocco)
        {
            _dataBuffer.fill(0);
            // Simula un CD da ~650MB (330.000 settori LBA da 2048 byte)
            const uint32_t totalLba = 330000;
            const uint32_t blockSize = 2048;

            // Formato Big-Endian obbligatorio per SCSI
            _dataBuffer[0] = (totalLba >> 24) & 0xFF;
            _dataBuffer[1] = (totalLba >> 16) & 0xFF;
            _dataBuffer[2] = (totalLba >> 8) & 0xFF;
            _dataBuffer[3] = totalLba & 0xFF;

            _dataBuffer[4] = (blockSize >> 24) & 0xFF;
            _dataBuffer[5] = (blockSize >> 16) & 0xFF;
            _dataBuffer[6] = (blockSize >> 8) & 0xFF;
            _dataBuffer[7] = blockSize & 0xFF;

            _dataIndex = 0;
            _dataLength = 8;
            _dataInPhase = true;
            _status = 0x58; // DRQ Attivo
            break;
        }

        case 0x43: // READ TOC (Table of Contents del CD)
        {
            _dataBuffer.fill(0);
            _dataBuffer[0] = 0x00; // TOC Length MSB
            _dataBuffer[1] = 0x0A; // TOC Length LSB (12 byte totali)
            _dataBuffer[2] = 0x01; // Prima traccia
            _dataBuffer[3] = 0x01; // Ultima traccia

            // Descrittore Traccia 1 (Traccia Dati)
            _dataBuffer[4] = 0x00;
            _dataBuffer[5] = 0x14; // ADR/Control (0x14 = Data Track, copy permitted)
            _dataBuffer[6] = 0x01; // Traccia 1
            _dataBuffer[7] = 0x00; // Riservato
            // LBA Inizio = 0
            _dataBuffer[8]  = 0x00;
            _dataBuffer[9]  = 0x00;
            _dataBuffer[10] = 0x00;
            _dataBuffer[11] = 0x00;

            _dataIndex = 0;
            _dataLength = 12;
            _dataInPhase = true;
            _status = 0x58; // DRQ Attivo
            break;
        }

        default:
        {
            // Comando SCSI non gestito -> Rifiuto (ILLEGAL REQUEST)
            _error = 0x50;  // Sense Key: Illegal Request
            _status = 0x51; // DRDY | ERR
            break;
        }
    }
}
