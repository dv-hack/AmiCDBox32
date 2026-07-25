#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

// -----------------------------------------------------------------------------
// COMMON CONFIGURATION
// -----------------------------------------------------------------------------
#define CFG_TUSB_MCU                OPT_MCU_RP2040

#define CFG_TUSB_OS                 OPT_OS_PICO
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_SPEED_FULL)
// -----------------------------------------------------------------------------
// DEVICE CONFIGURATION
// -----------------------------------------------------------------------------
#define CFG_TUD_ENABLED             1

#define CFG_TUD_ENDPOINT0_SIZE      64


// Abilita la classe Mass Storage (MSC) impostando a 1 le sue interfacce
#define CFG_TUD_MSC                 1

// Disabilita tutte le altre classi (CDC, HID, MIDI, ecc.)
#define CFG_TUD_CDC                 0
#define CFG_TUD_HID                 0
#define CFG_TUD_MIDI                0
#define CFG_TUD_VENDOR              0

// Configurazione buffer per MSC (2048 byte = 1 settore CD-ROM standard)
#define CFG_TUD_MSC_EP_BUFSIZE      2048

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */