#include "tusb.h"

// -----------------------------------------------------------------------------
// 1. DESCRITTORE DI DISPOSITIVO (Device Descriptor)
// -----------------------------------------------------------------------------
// Dichiara le specifiche di base: Vendor ID, Product ID e versione USB.
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200, // USB 2.0
    .bDeviceClass       = 0x00,   // La classe è definita a livello di interfaccia
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    
    .idVendor           = 0xCafe, // VID generico di test TinyUSB
    .idProduct          = 0x4000, // PID (arbitrario per MSC)
    .bcdDevice          = 0x0100, // Versione hardware 1.00

    .iManufacturer      = 0x01,   // Indice della stringa Manufacturer (vedi sotto)
    .iProduct           = 0x02,   // Indice della stringa Product
    .iSerialNumber      = 0x03,   // Indice della stringa Serial
    
    .bNumConfigurations = 0x01
};

// Callback invocata da TinyUSB per ottenere il Device Descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

// -----------------------------------------------------------------------------
// 2. DESCRITTORE DI CONFIGURAZIONE (Configuration Descriptor)
// -----------------------------------------------------------------------------
// Definisce le interfacce e gli Endpoint. MSC usa due Endpoint (uno IN e uno OUT).
enum
{
    ITF_NUM_MSC = 0,
    ITF_NUM_TOTAL
};

#define EPNUM_MSC_OUT   0x01
#define EPNUM_MSC_IN    0x81
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

uint8_t const desc_configuration[] =
{
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64)
};

// Callback invocata da TinyUSB per ottenere il Configuration Descriptor
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

// -----------------------------------------------------------------------------
// 3. DESCRITTORI DI STRINGA (String Descriptors)
// -----------------------------------------------------------------------------
// Il testo leggibile che comparirà nel gestore dispositivi del PC
char const* string_desc_arr [] =
{
    (const char[]) { 0x09, 0x04 }, // 0: Supported language is English (0x0409)
    "AmiCDBox32",                  // 1: Manufacturer
    "Virtual CD-ROM Drive",        // 2: Product
    "123456",                      // 3: Serials
};

static uint16_t _desc_str[32];

// Callback invocata da TinyUSB per ottenere le stringhe descrittive
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    uint8_t chr_count;

    if ( index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

        const char* str = string_desc_arr[index];
        chr_count = (uint8_t) strlen(str);
        if ( chr_count > 31 ) chr_count = 31;

        for(uint8_t i=0; i<chr_count; i++)
        {
            _desc_str[1+i] = str[i];
        }
    }

    // Primo byte: lunghezza, Secondo byte: tipo stringa (0x03)
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));

    return _desc_str;
}