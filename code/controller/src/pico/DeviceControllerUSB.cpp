// DeviceControllerUSB.cpp
//
#include "DeviceControllerUSB.hpp"

#include "Disc.hpp"
#include "Mounter.hpp"
#include "VirtualNavDisc.hpp"

#include "LedDebugger.hpp"

#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"
#include "class/msc/msc.h"
#include "class/msc/msc_device.h"

#include <cstring>
#include <iostream>

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
    setActivityLed(false);
    if(!gController) 
    {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x04, 0x01);
        return -1;
    }

    uint8_t opcode = scsi_cmd[0];

    // BYPASS control commands (tbd)
    if (opcode == SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL ||
        opcode == SCSI_CMD_MODE_SENSE_6 ||
        opcode == SCSI_CMD_MODE_SENSE_10 ||
        opcode == SCSI_CMD_GET_CONFIGURATION ||
        opcode == SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION ||
        opcode == SCSI_CMD_READ_DISC_INFORMATION)
    {
        return 0; 
    }

    setActivityLed(true);

    constexpr uint16_t MAX_HARDWARE_RAM_LIMIT = CFG_TUD_MSC_EP_BUFSIZE;
    uint16_t safeBufSize = (bufsize > MAX_HARDWARE_RAM_LIMIT) ? MAX_HARDWARE_RAM_LIMIT : bufsize;

    uint32_t bytesTransferred = 0;
    auto status = gController->drive().scsi_executeCommandDescriptorBlock(scsi_cmd, (uint8_t*)buffer, safeBufSize, bytesTransferred);
    
    setActivityLed(false);
    if (status == SCSI::Status::Good)
        return bytesTransferred; 
    
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
}

// ------------------------------------------------------------------------------------------------
// 1. Callback per verificare se il lettore è pronto
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    (void)lun;
    // True = Disco inserito e pronto; False = Nessun disco / Cassetto aperto
    return gController && gController->drive().isReady(); 
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
}

// ------------------------------------------------------------------------------------------------
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    (void)lun; (void)offset;

    if(!gController || !gController->drive().disc())
        return -1;

    setActivityLed(true);
    const bool ok = gController->drive().disc()->readSectors(lba, bufsize / SectorSize::ISO, SectorSize::ISO, (uint8_t*)buffer, bufsize);
    setActivityLed(false);

    if (!ok)
        return -1;
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

    _drive.setAudioPlayer(&_pwmBuzzerPlayer);

    //_drive.insertDisc( new VirtualNavDisc( FileSystem::Path("0:/"), "SD_NAVIGATOR" ) );


    //const auto mounted = _drive.mount("0:/Akira/Akira (Europe).cue");
    //const auto mounted = _drive.insertDisc("0:/AmigaOS3.2CD.iso");
;
    //_drive.insertDisc( Mounter::mount( "0:/AmigaOS3.2CD.iso" ) ); // OK!
    //_drive.insertDisc( Mounter::mount( "0:/Benefactor/Benefactor (Europe).cue" ) );// error reading track 17 and 18
    //_drive.insertDisc( Mounter::mount( "0:/Alien Breed 3D/Alien Breed 3D (Europe).cue" ) );
    //_drive.insertDisc( Mounter::mount( "0:/Akira/Akira (Europe).cue" ) );
    

    setReadyLed( _playlist.scanDirectory(FileSystem::Path("0:/") ) );
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerUSB::update()
{
    tud_task(); // update tinyUSB engine

    const auto mountEvent = _buttonMount.update();
    if( mountEvent == PhysicalButton::Event::ShortPress )
    {
        std::cout << "[SYSTEM] Eject pressed: unmounting disc...\n";
        auto d = _drive.eject();
        delete d;
    } 
    else if ( mountEvent == PhysicalButton::Event::LongPress )
    {
        std::cout << "[SYSTEM] Eject long press: Switching to VirtualNavDisc...\n";
        auto d = _drive.eject();
        delete d;
        _drive.insertDisc( new VirtualNavDisc( FileSystem::Path("0:/"), "SD_NAVIGATOR" ) );
    }
    
    
    const auto nextEvent  = _buttonNext.update();
    if( nextEvent == PhysicalButton::Event::ShortPress )
    {
        auto nextDisc = _playlist.nextEntry();        
        std::cout << "[SYSTEM] Next pressed: Mounting " << nextDisc.string() << "\n";
        auto d = _drive.eject();
        delete d;
        _drive.insertDisc( Mounter::mount( nextDisc.string() ) );
    } 
    else if ( nextEvent == PhysicalButton::Event::LongPress )
    {
        std::cout << "[SYSTEM] Next long press:\n";
    }


    if( _drive.state() == CdDrive::State::PlayingAudio )
        _drive.updateAudioPlayback();
}