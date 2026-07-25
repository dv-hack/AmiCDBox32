// TinyUSBHandlers.cpp
//
#include "TinyUSBHandlers.hpp"

#include "tusb_config.h"
#include "tusb.h"

#include "class/msc/msc.h"
#include "class/msc/msc_device.h"

#include <cstring>


// Aggiungiamo i comandi CD-ROM (standard SCSI MMC) mancanti in TinyUSB
#ifndef SCSI_CMD_READ_TOC
#define SCSI_CMD_READ_TOC 0x43
#endif

extern "C" {


void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void) lun; // Logical Unit Number (ignorato, ne abbiamo solo uno)

    const char vid[] = "AmiCDBox";
    const char pid[] = "CD-ROM Emulator ";
    const char rev[] = "1.0 ";

    memcpy(vendor_id  , vid, strlen(vid));
    memcpy(product_id , pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

// Callback invocata per ogni comando SCSI
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    void const* response_data = NULL;
    uint16_t response_len = 0;
    
    // Il primo byte è SEMPRE l'Opcode del comando SCSI
    uint8_t command = scsi_cmd[0];

    switch (command)
    {
        case SCSI_CMD_TEST_UNIT_READY: // 0x00
            // Il PC chiede "Ci sei? Sei pronto?"
            // Qui controllerai se una ISO è attualmente montata tramite la tua logica
            // Se sì -> return 0 (Pronto). Se no -> imposta un errore "Medium Not Present".
            response_len = 0; 
            break;

        case SCSI_CMD_READ_CAPACITY_10: // 0x25
            // Il PC chiede quanti settori ci sono sul CD
            // Devi rispondere con il numero di blocchi totali della ISO montata
            // response_data = tua_funzione_get_capacity();
            // response_len = 8;
            break;

        case SCSI_CMD_READ_10: // 0x28
            // IL COMANDO RE. Il PC vuole leggere i dati veri e propri.
            {
                // Estraiamo il LBA (Logical Block Address) richiesto dai byte 2,3,4,5
                uint32_t lba = (scsi_cmd[2] << 24) | (scsi_cmd[3] << 16) | 
                               (scsi_cmd[4] <<  8) | (scsi_cmd[5]);
                
                // Estraiamo quanti blocchi vuole leggere (byte 7,8)
                uint16_t block_count = (scsi_cmd[7] << 8) | scsi_cmd[8];

                // Chiamata alla tua classe C++ che legge dalla SD Card
                // cdDrive->readSectors(lba, block_count, buffer);
                
                response_len = block_count * 2048; // Standard CD-ROM sector size
            }
            break;

        case SCSI_CMD_READ_TOC: // 0x43
            // Il PC vuole sapere la geometria del disco (Traccia 1 Dati, Traccia 2 Audio, ecc.)
            // Qui passerai la CdMap generata dal parsing del file .cue
            break;

        default:
            // Comando non supportato: il PC (o l'Amiga) gestirà elegantemente il rifiuto
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            return -1;
    }

    // Se abbiamo dati da restituire (es. per la TOC o l'INQUIRY), li copiamo nel buffer
    if ( response_data && response_len > 0 )
    {
        if(bufsize < response_len) response_len = bufsize;
        memcpy(buffer, response_data, response_len);
    }

    return response_len; // Restituisce i byte effettivamente elaborati/inviati
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
    *block_size = 2048;       // Standard CD-ROM sector size
    *block_count = 300000;    // Valore fittizio di test (es. ~600MB di CD)
}

// ------------------------------------------------------------------------------------------------
// 3. Callback per la lettura dei settori (Qui poi collegherai il tuo CdDrive)
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    (void)lun;
    (void)offset;

    // Stub temporaneo di test: riempie il buffer di zeri o dati fittizi
    memset(buffer, 0, bufsize);
    
    return bufsize; // Restituisce i byte letti
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
