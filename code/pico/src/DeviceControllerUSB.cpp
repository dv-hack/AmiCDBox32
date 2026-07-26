// DeviceControllerUSB.cpp
//
#include "DeviceControllerUSB.hpp"

#include "LedDebugger.hpp"

#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"

#include "class/msc/msc.h"
#include "class/msc/msc_device.h"

#include <cstring>

static DeviceControllerUSB *gController{nullptr};

extern "C" {
    
#ifndef SCSI_CMD_READ_TOC
#define SCSI_CMD_READ_TOC 0x43
#endif
#ifndef SCSI_CMD_MODE_SENSE_6
#define SCSI_CMD_MODE_SENSE_6 0x1A
#endif
#ifndef SCSI_CMD_MODE_SENSE_10
#define SCSI_CMD_MODE_SENSE_10 0x5A
#endif
#ifndef SCSI_CMD_GET_CONFIGURATION
#define SCSI_CMD_GET_CONFIGURATION 0x46
#endif
#ifndef SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL
#define SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL 0x1E
#endif

//static constexpr int kBlockCount_DEBUG = 300000;    // Valore fittizio di test (es. ~600MB di CD)
static constexpr int kBlockCount_DEBUG = 38037;    // Valore fittizio di test (es. ~60MB di CD)

// ------------------------------------------------------------------------------------------------
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    //setActivityLed(true);
    (void) lun; // Logical Unit Number (ignorato, ne abbiamo solo uno)

    const char vid[] = "AmiCDBox";
    const char pid[] = "CD-ROM Emulator ";
    const char rev[] = "1.0 ";

    memcpy(vendor_id  , vid, strlen(vid));
    memcpy(product_id , pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
    //setActivityLed(false);
}

// ------------------------------------------------------------------------------------------------
// Callback invocata per i comandi SCSI non standard o non gestiti da TinyUSB
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    setErrorLed(false);
    setActivityLed(false);

    void const* response_data = NULL;
    uint16_t response_len = 0;
    
    uint8_t command = scsi_cmd[0];

    switch (command)
    {
        case SCSI_CMD_READ_TOC: // 0x43
        {
            static uint8_t toc[20] = {
                0x00, 0x12, 
                0x01,       
                0x01,       
                0x00, 0x14, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x14, 0xAA, 0x00, 
                (uint8_t)(kBlockCount_DEBUG >> 24), (uint8_t)(kBlockCount_DEBUG >> 16), 
                (uint8_t)(kBlockCount_DEBUG >> 8),  (uint8_t)(kBlockCount_DEBUG & 0xFF) 
            };
            
            response_data = toc;
            response_len = sizeof(toc);
            setActivityLed(true);
            break;
        }

        // Comandi di controllo comuni che Windows invia ai CD-ROM: li accettiamo silenziosamente
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL: // 0x1E
        case SCSI_CMD_MODE_SENSE_6:                 // 0x1A
        case SCSI_CMD_MODE_SENSE_10:                // 0x5A
        case SCSI_CMD_GET_CONFIGURATION:            // 0x46
        {
            response_len = 0; // Nessun dato o operazione riuscita
            break;
        }

        default:
        {
            // Per veri comandi sconosciuti, manteniamo il blocco ma senza accendere il LED fisso per i controlli standard
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x24, 0x00);
            //setErrorLed(true);
            return -1;
        }
    }

    if ( response_data && response_len > 0 )
    {
        if(bufsize < response_len) response_len = bufsize;
        memcpy(buffer, response_data, response_len);
    }

    return response_len; 
}

// ------------------------------------------------------------------------------------------------
// 1. Callback per verificare se il lettore è pronto
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    (void)lun;
    // True = Disco inserito e pronto; False = Nessun disco / Cassetto aperto
    return true; 
}

// ------------------------------------------------------------------------------------------------
// 2. Callback per restituire la capacità totale del disco (Numero di blocchi e dimensione blocco)
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
    (void)lun;
    
    if (gController) {
        *block_count = gController->drive().disc().getTotalSectors();
    } else {
        *block_count = 0;
    }
}

// ------------------------------------------------------------------------------------------------
// 3. Callback per la lettura dei settori
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    setErrorLed(false);
    setActivityLed(true);

    (void)lun;
    (void)offset;

    uint16_t block_count = bufsize / 2048;

    if (!gController || !gController->drive().readDataToBuffer(lba, block_count, (uint8_t*)buffer))
    {
        //setActivityLed(false);
        //setErrorLed(true);
        return -1;
    }   

    setActivityLed(false);
    setErrorLed(false);
    return bufsize;
}

// ------------------------------------------------------------------------------------------------
// 4. Callback per la scrittura (I CD-ROM sono di sola lettura, restituiamo 0)
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    (void)lun;
    (void)lba;
    (void)offset;
    (void)buffer;
    (void)bufsize;
    
    return -1; // Non supportato / Sola lettura
}

}

// ------------------------------------------------------------------------------------------------
DeviceControllerUSB::DeviceControllerUSB()
{
    gController = this;
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerUSB::init()
{
    board_init();
    tusb_init();

    _sdCard.mount();

    //const auto mounted = _drive.mount("0:/Akira/Akira (Europe).cue");
    const auto mounted = _drive.mount("0:/AmigaOS3.2CD.iso");

    setReadyLed( mounted );
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerUSB::update()
{
    // Motore TinyUSB (gestisce l'handshake in background e chiama le callback)
    tud_task(); 
}