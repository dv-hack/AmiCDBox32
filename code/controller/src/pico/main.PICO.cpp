// main.cpp
//
#include "pico/stdlib.h"
#include "hw_config.h"
#include "spi.h"
#include "sd_card.h"
#include "diskio.h"

#define TINY_USB_MODE
#if defined TINY_USB_MODE
#include "DeviceControllerUSB.hpp"
#else
#include "DeviceControllerGPIO.hpp"
#endif

#include "LedDebugger.hpp"


// 1. Configurazione del blocco hardware SPI (SPI 0)
static spi_t spis[] = {
    {
        .hw_inst = spi0,           // Istanza hardware del Pico
        .miso_gpio = 16,           // GP16
        .mosi_gpio = 19,           // GP19
        .sck_gpio = 18,            // GP18
        .baud_rate = 12500 * 1000, // 12.5 MHz (ottimale per SD)
        .set_drive_strength = false
    }
};

// 2. Configurazione della singola scheda SD
static sd_card_t sd_cards[] = {
    {
        .pcName = "0:",            // Nome del volume logico per FatFS
        .spi = &spis[0],           // Punta all'istanza SPI sopra
        .ss_gpio = 17,             // Chip Select (CS) su GP17
        .use_card_detect = false,   // Non usiamo il pin CD fisico del lettore
        .card_detect_gpio = 0,
        .card_detected_true = 0,
        .m_Status = STA_NOINIT,
        .mounted = false
    }
};

extern "C" size_t spi_get_num() { return sizeof(spis) / sizeof(spis[0]); }
extern "C" spi_t *spi_get_by_num(size_t num) {
    if (num < spi_get_num()) return &spis[num];
    return nullptr;
}

extern "C" size_t sd_get_num() { return sizeof(sd_cards) / sizeof(sd_cards[0]); }
extern "C" sd_card_t *sd_get_by_num(size_t num) {
    if (num < sd_get_num()) return &sd_cards[num];
    return nullptr;
}

#define TINYUSB_MODE

int main()
{
    
#if defined TINY_USB_MODE
    DeviceController* controller = new DeviceControllerUSB;
#else
    DeviceController* controller = new DeviceControllerGPIO;
#endif

    LedDebugger::instance().init();


    controller->init();
    controller->updateLoop();
    
    
    delete controller;
}