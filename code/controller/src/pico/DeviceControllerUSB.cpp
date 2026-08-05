// DeviceControllerUSB.cpp
//
#include "DeviceControllerUSB.hpp"

#include "Disc.hpp"
#include "VirtualNavDisc.hpp"

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
#ifndef SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION
#define SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION 0x4A
#endif
#ifndef SCSI_CMD_READ_DISC_INFORMATION
#define SCSI_CMD_READ_DISC_INFORMATION 0x51
#endif

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
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    setErrorLed(false);
    if(!gController) 
    {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x04, 0x01);
        return -1;
    }

    uint8_t opcode = scsi_cmd[0];

    // BYPASS: Comandi di controllo a cui dobbiamo rispondere "Tutto OK" senza fare nulla
    if (opcode == SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL ||
        opcode == SCSI_CMD_MODE_SENSE_6 ||
        opcode == SCSI_CMD_MODE_SENSE_10 ||
        opcode == SCSI_CMD_GET_CONFIGURATION ||
        opcode == SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION || // <-- FONDAMENTALE PER WINDOWS
        opcode == SCSI_CMD_READ_DISC_INFORMATION)
    {
        return 0; 
    }

    uint32_t bytesTransferred = 0;
    auto status = gController->drive().scsi_executeCommandDescriptorBlock(scsi_cmd, (uint8_t*)buffer, bytesTransferred);

    if (status == SCSI::Status::Good)
        return bytesTransferred;
    
    // // Se il tuo VirtualNavDisc (o CdDrive) non supporta bene READ_TOC (0x43), fermiamoci per vederlo!
    // if (opcode == SCSI_CMD_READ_TOC) 
    //     LedDebugger::instance().visualPanic(3); // 3 LAMPEGGI = READ_TOC fallito

    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
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
    if(!gController || !gController->drive().disc())
    {
        *block_count = 0;
        *block_size = 2048;
        return;
    }
    
    auto disc = gController->drive().disc();
    *block_count = disc->totalSectors();
    *block_size = 2048; // Forza a 2048 per i CD-ROM

    // // TRAPPOLA: Se il disco dichiara 0 settori, Windows lo considera vuoto
    // if (*block_count == 0) 
    //     LedDebugger::instance().visualPanic(2); // 2 LAMPEGGI = totalSectors() sta tornando 0!
}

// ------------------------------------------------------------------------------------------------
// 3. Callback per la lettura dei settori
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    (void)lun; (void)offset;

    if(!gController || !gController->drive().disc())
        return -1;

    setActivityLed(true);
    const bool ok = gController->drive().disc()->readSectors(lba, bufsize / 2048, SectorSize::ISO, (uint8_t*)buffer);
    setActivityLed(false);

    if (!ok)
        return -1;

    // if (lba == 16) 
    // {
    //     uint8_t* b = (uint8_t*)buffer;
    //     // La stringa magica a offset 1 DEVE essere "CD001"
    //     if (b[1] != 'C' || b[2] != 'D' || b[3] != '0' || b[4] != '0' || b[5] != '1') 
    //         LedDebugger::instance().visualPanic(4); // 4 LAMPEGGI = L'LBA 16 non contiene la firma ISO corretta
    // }

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

    FileSystem::init();

    _drive.insertDisc( new VirtualNavDisc( FileSystem::Path("0:/"), "SD_NAVIGATOR" ) );


    //const auto mounted = _drive.mount("0:/Akira/Akira (Europe).cue");
    //const auto mounted = _drive.insertDisc("0:/AmigaOS3.2CD.iso");

    setReadyLed( true);
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerUSB::update()
{
    // Motore TinyUSB (gestisce l'handshake in background e chiama le callback)
    tud_task(); 
}