// main.cpp
//
#include "DeviceController.hpp"

#include "pico/stdlib.h"
#include "hw_config.h"
#include "spi.h"
#include "sd_card.h"
#include "diskio.h"

#define TINY_USB_MODE
#if defined TINY_USB_MODE
#include "bsp/board.h"
#include "tusb.h"
#endif

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

// 3. I metodi di "getter" che il linker non trovava
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
    board_init();
    //stdio_init_all();
    tusb_init();

    // Inizializza la tua logica di lettura SD e CdMap
    // cdDrive->init();

    while (1)
    {
        // Motore TinyUSB (gestisce l'handshake in background e chiama le callback)
        tud_task(); 
        
        // Se volessi gestire un bottone hardware fisico o la UI
        // uiController->update();
    }    

#else
    stdio_init_all();
    sleep_ms(2000); // Allow USB serial to hook up

    DeviceController controller;
    controller.init();

    while (true)
    {
        controller.update();
    }
#endif
}