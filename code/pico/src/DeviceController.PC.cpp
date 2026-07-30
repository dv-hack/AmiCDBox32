// DeviceController.cpp
//
#include "DeviceController.PC.hpp"

#include "Disc.hpp"
#include "Mounter.hpp"
#include "Endianess.hpp"
#include "ISO9660Parser.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static inline std::vector<std::string> tokenize(const std::string &s)
{
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string curr;
    while (getline(ss, curr, ' '))
        tokens.push_back(curr);
    return tokens;
}
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
void DeviceControllerPC::init()
{
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerPC::update()
{
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerPC::updateLoop()
{
    while (true)
    {
        std::cout << "[AmiCDBox32] Enter Command >> ";

        std::string command;
        std::getline(std::cin, command);

        if (command.starts_with("mount"))         cmd_mount(tokenize(command)[1]);
        else if (command.starts_with("unmount"))  cmd_unmount();
        else if (command.starts_with("read"))     cmd_read();
        else if (command.starts_with("dir"))      cmd_showContents();
        else if (command == "q")
            break;

        update();
    }
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerPC::cmd_mount(const std::string &filename)
{
    std::cout << "Mounting " << filename << "...\n";

    Disc disc = Mounter::mount(filename);
    _drive.insertDisc(std::move(disc));
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerPC::cmd_unmount()
{
    _drive.eject();
}

// ------------------------------------------------------------------------------------------------
void DeviceControllerPC::cmd_read()
{
    if (!_drive.isReady())
    {
        std::cout << "Read failed. no disc." << '\n';
        return;
    }

    auto readCapacity = [&]
    {
        uint8_t readCapCdb[10] = {SCSI::READ_CAPACITY, 0};
        uint8_t responseBuffer[2048] = {0};
        uint32_t bytesTransferred = 0;

        if (auto ec = _drive.scsi_executeCommandDescriptorBlock(readCapCdb, responseBuffer, bytesTransferred) != SCSI::Status::Good)
        {
            std::cout << "Error reading capacity: " << static_cast<int>(ec) << "\n";
            return;
        }

        const uint32_t lastLba = Endianess::readBE32(&responseBuffer[0]);
        const uint32_t blockSize = Endianess::readBE32(&responseBuffer[4]);

        std::cout << std::dec << "[SCSI OK] Read Capacity -> Settori Totali: " << (lastLba + 1)
                  << ", Block Size: " << blockSize << " byte" << std::endl;
    };

    auto readTOC = [&]
    {
        uint8_t outBuffer[256];

        uint8_t cdb[10];
        cdb[0] = SCSI::READ_TOC;
        cdb[1] = 0x00;
        cdb[6] = 0x01;
        Endianess::writeBE16(&cdb[7], sizeof(outBuffer));

        uint32_t bytesTransferred = 0;

        if (auto ec = _drive.scsi_executeCommandDescriptorBlock(cdb, outBuffer, bytesTransferred) != SCSI::Status::Good)
        {
            std::cout << "Error reading TOC: " << static_cast<int>(ec) << "\n";
            return;
        }


        const uint16_t tocDataLength = Endianess::readBE16(&outBuffer[0]);
        const uint8_t firstTrack = outBuffer[2];
        const uint8_t lastTrack = outBuffer[3];

        std::cout << "=== SCSI READ_TOC ===" << std::endl;
        std::cout << "Data Length : " << std::dec << tocDataLength << " byte" << std::endl;
        std::cout << "First Track : " << static_cast<int>(firstTrack) << std::endl;
        std::cout << "Last Track  : " << static_cast<int>(lastTrack) << std::endl;
        std::cout << "Bytes Trsf  : " << std::dec << bytesTransferred << " byte" << std::endl;
        std::cout << "-------------------------------" << std::endl;

        // Parse track descriptors (8 byte each, starti from offset 4)
        uint32_t offset = 4;
        while (offset + 8 <= bytesTransferred)
        {
            uint8_t controlFlags = outBuffer[offset + 1];
            uint8_t trackNum = outBuffer[offset + 2];
            uint32_t lba = Endianess::readBE32(&outBuffer[offset + 4]);

            bool isDataTrack = (controlFlags & 0x04) != 0; // Bit 2 = 1 -> Data Track (Mode 1)

            if (trackNum == 0xAA)
            {
                std::cout << "[Lead-Out / End-of-Disc] LBA: " << lba
                          << " (Totale settori ISO: " << lba << ")" << std::endl;
            }
            else
            {
                std::cout << "Traccia " << static_cast<int>(trackNum)
                          << " | Start LBA: " << lba
                          << " | Tipo: " << (isDataTrack ? "DATA" : "AUDIO") << std::endl;
            }

            offset += 8;
        }
    };

    auto read10 = [&]
    {
        uint8_t sectorBuffer[2048];

        uint8_t cdb[10] = {0};

        cdb[0] = SCSI::READ_10;            // SCSI Opcode: READ_10
        cdb[1] = 0x00;                     // Flags
        Endianess::writeBE32(&cdb[2], 16); // LBA 16 (Big-Endian offset 2..5)
        cdb[6] = 0x00;                     // Group / Reserved
        Endianess::writeBE16(&cdb[7], 1);  // Transfer Length: 1 sector (offset 7..8)
        cdb[9] = 0x00;                     // Control

        uint32_t bytesTransferred = 0;
        SCSI::Status status = _drive.scsi_executeCommandDescriptorBlock(cdb, sectorBuffer, bytesTransferred);

        assert(status == SCSI::Status::Good && "READ_10 ha restituito CheckCondition!");

        std::cout << "[OK] Lettura SCSI completata. Byte ricevuti: " << bytesTransferred << std::endl;

        // 6. Verify structure of Primary Volume Descriptor (ISO 9660)
        // Offset 0: Volume Descriptor Type (1 = Primary Volume Descriptor)
        uint8_t descriptorType = sectorBuffer[0];
        assert(descriptorType == 1 && "LBA 16 does not contain a valid Primary Volume Descriptor!");

        // Offset 1..5: Standard Identifier ("CD001")
        char standardId[6] = {0};
        std::memcpy(standardId, &sectorBuffer[1], 5);

        assert(std::strcmp(standardId, "CD001") == 0 && "Magic bytes 'CD001' not found!");
        std::cout << "[OK] Magic Bytes found: \"" << standardId << "\"" << std::endl;

        // Offset 40..71: Volume Identifier (32 byte, space padded)
        char rawVolName[33] = {0};
        std::memcpy(rawVolName, &sectorBuffer[40], 32);

        std::string volName(rawVolName);
        size_t lastChar = volName.find_last_not_of(' ');
        if (lastChar != std::string::npos)
            volName.erase(lastChar + 1);

        std::cout << "[OK] Volume Identifier: \"" << volName << "\"" << std::endl;
    };

    readCapacity();
    readTOC();
    read10();
}


void DeviceControllerPC::cmd_showContents()
{
    if (!_drive.isReady())
    {
        std::cout << "Read failed. no disc." << '\n';
        return;
    }

    std::cout<< "\n------------------------------------------------\n";
    std::cout<< "[" << Iso9660Parser::getVolumeName(_drive.disc() ) << "]";
    std::cout<< "\n------------------------------------------------\n";


    auto contents = Iso9660Parser::getRootDirectory(_drive.disc());
    for( auto &item : contents )
    {
        if( item.isDirectory )
            std::cout << item.name << "/" << '\n';
        else
            std::cout << "   " << item.name << "      " << item.size << " bytes\n";
    }
}