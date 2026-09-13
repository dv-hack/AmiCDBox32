// IDEController.cpp
//
#include "IDEController.hpp"

#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "ide_bus.pio.h"

 #include <iostream>

IDEController* IDEController::s_instance = nullptr;

IDEController::IDEController()
{
    s_instance = this;
}

IDEController::~IDEController()
{
    if (_pio)
    {
        // 1. Disabilita gli interrupt associati al PIO
        const uint irqNum = (_pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0;
        irq_set_enabled(irqNum, false);
        pio_set_irq0_source_enabled(_pio, pis_interrupt0, false);
    
        // 2. Ferma, disassocia e rimuove la State Machine di Lettura (DIOR#)
        pio_sm_set_enabled(_pio, _smR, false);
        pio_sm_unclaim(_pio, _smR);
        pio_remove_program(_pio, &ide_strobe_dior_program, _offsetR);
    
        // 3. Ferma, disassocia e rimuove la State Machine di Scrittura (DIOW#)
        pio_sm_set_enabled(_pio, _smW, false);
        pio_sm_unclaim(_pio, _smW);
        pio_remove_program(_pio, &ide_strobe_diow_program, _offsetW);
    }

    s_instance = nullptr;
}

void IDEController::init(PIO pioInstance)
{
    _pio = pioInstance;

    // 1. Configurazione GPIO Bus Dati (GP00..GP07) come ingressi iniziali
    for (int pin = Pin::GP00; pin <= Pin::GP07; ++pin)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // 2. Configurazione Pin di Controllo Transceiver (GP08, GP09, GP10)
    gpio_init(Pin::GP08); gpio_set_dir(Pin::GP08, GPIO_OUT); // OE_LOW#
    gpio_init(Pin::GP09); gpio_set_dir(Pin::GP09, GPIO_OUT); // OE_HIGH#
    gpio_init(Pin::GP10); gpio_set_dir(Pin::GP10, GPIO_OUT); // DIR

    // Disabilita inizialmente i transceiver e imposta direzione da Amiga -> Pico
    enableTransceivers(false, false);
    setTransceiverDirection(false);

    // 3. Configurazione Pin di Controllo e Indirizzo (GP11..GP17)
    for (int pin = Pin::GP11; pin <= Pin::GP17; ++pin)
    {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // 4. Inizializza SM per DIOR# (Lettura)
    _offsetR = pio_add_program(_pio, &ide_strobe_dior_program);
    _smR = pio_claim_unused_sm(_pio, true);
    pio_sm_config cR = ide_strobe_dior_program_get_default_config(_offsetR);
    sm_config_set_in_pins(&cR, Pin::GP11);
    pio_sm_init(_pio, _smR, _offsetR, &cR);
    pio_sm_set_enabled(_pio, _smR, true);

    // 5. Inizializza SM per DIOW# (Scrittura)
    _offsetW = pio_add_program(_pio, &ide_strobe_diow_program);
    _smW = pio_claim_unused_sm(_pio, true);
    pio_sm_config cW = ide_strobe_diow_program_get_default_config(_offsetW);
    sm_config_set_in_pins(&cW, Pin::GP11);
    pio_sm_init(_pio, _smW, _offsetW, &cW);
    pio_sm_set_enabled(_pio, _smW, true);

    // 6. Aggancio interrupt globale PIO
    const uint irqNum = (_pio == pio0) ? PIO0_IRQ_0 : PIO1_IRQ_0;
    pio_set_irq0_source_enabled(_pio, pis_interrupt0, true);
    irq_set_exclusive_handler(irqNum, IDEController::pioInterruptHandler);
    irq_set_enabled(irqNum, true);

    std::cout << "IDEController initialized\n";
}

void IDEController::setTransceiverDirection(bool driveToAmiga)
{
    // GP10 (DIR): Low (0) = Amiga -> Pico, High (1) = Pico -> Amiga
    gpio_put(Pin::GP10, driveToAmiga ? 1 : 0);
}

void IDEController::enableTransceivers(bool enableLow, bool enableHigh)
{
    // Attivi bassi (0 = abilitato)
    gpio_put(Pin::GP08, enableLow ? 0 : 1);
    gpio_put(Pin::GP09, enableHigh ? 0 : 1);
}

void IDEController::pioInterruptHandler()
{
    if (!s_instance)
        return;

    pio_interrupt_clear(s_instance->_pio, 0);

    auto processRawSample = [](uint32_t rawSample, bool isWrite)
    {
        
        const bool cs0_pin = rawSample & (1u << 31);
        const bool cs1_pin = rawSample & (1u << 30);
        const bool a0_pin  = rawSample & (1u << 29);
        const bool a1_pin  = rawSample & (1u << 28);
        const bool a2_pin  = rawSample & (1u << 27);        
        
        //rawSample >>= 25; // Estrae i bit 25..31 (CS0, CS1, A0, A1, A2)
        // const bool cs0_pin = (rawSample & (1 << 0)); // GP11
        // const bool cs1_pin = (rawSample & (1 << 1)); // GP12
        // const bool a0_pin  = (rawSample & (1 << 2)); // GP13
        // const bool a1_pin  = (rawSample & (1 << 3)); // GP14
        // const bool a2_pin  = (rawSample & (1 << 4)); // GP15

        const bool cs0_active = !cs0_pin;
        const bool cs1_active = !cs1_pin;
        const uint8_t base_addr = (a2_pin << 2) | (a1_pin << 1) | a0_pin;

        IDETransaction tx{};
        {
            tx.isWrite = isWrite;
            tx.address = cs0_active 
                        ? base_addr 
                        : (cs1_active 
                          ? (0x08 | base_addr) 
                          : 0xFF); // 0xFF = Invalid
        }

        // std::cout << "RAW: 0x" << std::hex << rawSample 
        //   << " | CS0:" << cs0_pin << " CS1:" << cs1_pin 
        //   << " A0:" << a0_pin << " A1:" << a1_pin << " A2:" << a2_pin << std::dec << "\n";
        return tx;
    };

    // A. Controlla il FIFO di Lettura (DIOR#)
    while (!pio_sm_is_rx_fifo_empty(s_instance->_pio, s_instance->_smR))
    {
        const uint32_t rawSample = pio_sm_get(s_instance->_pio, s_instance->_smR);

        const auto tx = processRawSample(rawSample, false);
       
        if (tx.address != 0xFF)
            s_instance->processTransaction(tx);
    }

    // B. Controlla il FIFO di Scrittura (DIOW#)
    while (!pio_sm_is_rx_fifo_empty(s_instance->_pio, s_instance->_smW))
    {
        const uint32_t rawSample = pio_sm_get(s_instance->_pio, s_instance->_smW);
        const auto tx = processRawSample(rawSample, true);

        if (tx.address != 0xFF)
            s_instance->processTransaction(tx);
    }
}

// Imposta la direzione dei GPIO del Pico e del segnale DIR (GP10)
void IDEController::setBusDataDirection(bool driveToHost)
{
    if (driveToHost)
    {
        // 1. Configura GP00..GP07 come uscite sul Pico
        gpio_set_dir_masked(DATA_BUS_MASK, DATA_BUS_MASK);
        
        // 2. Imposta la direzione del transceiver 74LVC245 (DIR = 1: Pico -> Host)
        gpio_put(Pin::GP10, 1);
    }
    else
    {
        // 1. Disabilita prima i transceiver (Tri-state) per evitare conflitti sul bus
        enableTransceivers(false, false);

        // 2. Imposta la direzione del transceiver (DIR = 0: Host -> Pico)
        gpio_put(Pin::GP10, 0);

        // 3. Configura GP00..GP07 come ingressi sul Pico
        gpio_set_dir_masked(DATA_BUS_MASK, 0);
    }
}

// Scrive 8 bit sui pin GP00..GP07 in un solo ciclo di clock
void IDEController::writeDataByte(uint8_t byte)
{
    gpio_put_masked(DATA_BUS_MASK, byte);
}

// Legge 8 bit dai pin GP00..GP07
uint8_t IDEController::readDataByte()
{
    return static_cast<uint8_t>(gpio_get_all() & DATA_BUS_MASK);
}


void IDEController::processTransaction(const IDETransaction& tx)
{
    uint8_t traceData = 0;

    if (!tx.isWrite)
    {
        uint8_t valueToSend = 0x00;
        if( tx.address < _registers.size() )
            valueToSend = _registers[tx.address]; // Legge il valore dal registro interno

        switch (tx.address)
        {
            case 0x01: // Error Register
                valueToSend = 0x01; // No error
                break;
            case 0x04: // Cylinder Low (Firma ATAPI)
                valueToSend = 0x14; // Magic Byte Low
                break;
            case 0x05: // Cylinder High (Firma ATAPI)
                valueToSend = 0xEB; // Magic Byte High (0xEB14 = Dispositivo ATAPI CD-ROM)
                break;
            case 0x07: // Status Register CS0
            case 0x0E: // Status Register CS1
                valueToSend = 0x50; // DRIVE READY (0x40) + DRIVE SEEK COMPLETE (0x10)
                break;
            default:
                break;
        }
       
        setBusDataDirection(true);
        writeDataByte(valueToSend);
        enableTransceivers(true, false);

        traceData = valueToSend;
        // Attende la fine del ciclo (DIOR# alto) prima di rilasciare il bus
        while (gpio_get(Pin::GP16) == 0)
            tight_loop_contents();

        // Disabilita IMMEDIATAMENTE i transceiver per evitare bus contention
        enableTransceivers(false, false);
        setBusDataDirection(false);        
    }
    else
    {
        // Se è una scrittura dall'Host (DIOW#)
        setBusDataDirection(false);
        enableTransceivers(true, false); // Abilita ricezione D0..D7

        // Attende il fronte di salita di DIOW# per garantire dati stabili
        while (gpio_get(Pin::GP17) == 0)
            tight_loop_contents();
        
        uint8_t receivedByte = readDataByte();
        traceData = receivedByte;

        // Rilascia i transceiver a campionamento avvenuto
        enableTransceivers(false, false);        

        if (tx.address < 16)
            _registers[tx.address] = receivedByte; 
        
        // std::cout << "[IDE] Host wrote 0x" << std::hex << (int)receivedByte 
        //           << " to register 0x" << (int)tx.address << "\n";
    }
    
    // --- Salvataggio nel Ring Buffer per il debug ---
    size_t nextHead = (_traceHead + 1) % TRACE_BUFFER_SIZE;
    if (nextHead != _traceTail) // Se c'è spazio nel buffer
    {
        _traceBuffer[_traceHead] = {tx.address, tx.isWrite, traceData};
        _traceHead = nextHead;
    }    
}

void IDEController::update()
{
    // Macchina a stati di background per la gestione di trasferimenti DMA o blocchi dati estesi
    printTrace();
}


void IDEController::printTrace()
{
    while (_traceTail != _traceHead) 
    {
        TraceEvent e = _traceBuffer[_traceTail];
        _traceTail = (_traceTail + 1) % TRACE_BUFFER_SIZE;
        
        std::cout << "[IDE] " << (e.isWrite ? "WR" : "RD") 
                  << " Reg: 0x" << std::hex << (int)e.address 
                  << " | Data: 0x" << (int)e.data << std::dec << "\n";
    }
}