cmake_minimum_required(VERSION 3.13)

option(BUILD_FOR_PICO "Build with toolchain for Raspberry Pi Pico" ON)

if (BUILD_FOR_PICO)
    message(STATUS "Configuring toolchain for RASPBERRY PI PICO...")

    # Fallback mechanism for PICO_SDK_PATH (Windows friendly)
    if (NOT PICO_SDK_PATH)
        if (DEFINED ENV{PICO_SDK_PATH})
            set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
        else()
            # If using the VS Code Extension on Windows, it will pass this variable automatically.
            # Otherwise, you can hardcode your Windows path here as a last resort:
            # set(PICO_SDK_PATH "C:/Template/pico-sdk") 
            message(FATAL_ERROR "PICO_SDK_PATH is not set. Please configure it in VS Code settings or environment variables.")
        endif()
    endif()

    # Convert Windows backslashes to forward slashes to prevent CMake escaping issues
    file(TO_CMAKE_PATH "${PICO_SDK_PATH}" PICO_SDK_PATH)

    # Initialize the SDK using the clean path
    include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

    project(AmiCDBox32Project C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    
    pico_sdk_init()

    add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFS_SPI)
if (BUILD_FOR_TINYUSB)
    add_executable(AmiCDBox32
        src/pico.usb/main.PICO.cpp
        src/pico.usb/AudioPlayer.cpp
        src/pico.usb/AudioPlayer.hpp
        src/pico.usb/DeviceControllerUSB.cpp
        src/pico.usb/DeviceControllerUSB.hpp
        src/pico.usb/FileSystem.PICO.cpp
        src/pico.usb/LedDebugger.cpp
        src/pico.usb/LedDebugger.hpp
        src/pico.usb/PhysicalButton.cpp
        src/pico.usb/PhysicalButton.hpp
        src/pico.usb/PwmBuzzerPlayer.cpp
        src/pico.usb/PwmBuzzerPlayer.hpp
        src/pico.usb/tusb_config.h
        src/pico.usb/usb_descriptors.c
        src/CdDrive.cpp
        src/CdDrive.hpp
        src/Disc.cpp
        src/Disc.hpp
        src/DiscPlaylist.cpp
        src/DiscPlaylist.hpp        
        src/CueParser.cpp
        src/CueParser.hpp
        src/DeviceController.hpp
        src/Endianess.hpp
        src/FileSystem.hpp
        src/IAudioPlayer.hpp
        src/Mounter.cpp
        src/Mounter.hpp
        src/Sector.hpp
        src/Version.hpp
        src/Track.cpp
        src/Track.hpp
        src/VirtualNavDisc.cpp
        src/VirtualNavDisc.hpp
    )
else()    
    add_executable(AmiCDBox32
        src/pico.ide/main.PICO.cpp
        src/pico.ide/AudioPlayer.cpp
        src/pico.ide/AudioPlayer.hpp
        src/pico.ide/DeviceControllerGPIO.cpp
        src/pico.ide/DeviceControllerGPIO.hpp
        src/pico.ide/FileSystem.PICO.cpp
        src/pico.ide/PhysicalButton.cpp
        src/pico.ide/PhysicalButton.hpp
        src/pico.ide/Pins.hpp
        src/pico.ide/IDEController.cpp
        src/pico.ide/IDEController.hpp
        src/CdDrive.cpp
        src/CdDrive.hpp
        src/Disc.cpp
        src/Disc.hpp
        src/DiscPlaylist.cpp
        src/DiscPlaylist.hpp        
        src/CueParser.cpp
        src/CueParser.hpp
        src/DeviceController.hpp
        src/Endianess.hpp
        src/FileSystem.hpp
        src/IAudioPlayer.hpp
        src/Mounter.cpp
        src/Mounter.hpp
        src/Sector.hpp
        src/Version.hpp
        src/Track.cpp
        src/Track.hpp
        src/VirtualNavDisc.cpp
        src/VirtualNavDisc.hpp
    )
endif()
    target_compile_definitions(AmiCDBox32 PRIVATE TARGET_PICO=1)

    target_include_directories(AmiCDBox32 BEFORE PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/src/pico
    )

    # Set C++20 standard requirements
    target_compile_features(AmiCDBox32 PRIVATE cxx_std_20)



    # Link the standard Pico libraries (GPIO and Standard I/O)
    target_link_libraries(AmiCDBox32 
        pico_stdlib
        hardware_spi
        hardware_gpio
        hardware_pio
        hardware_irq

#        hardware_pwm
        FatFs_SPI
#        tinyusb_device
#        tinyusb_board
    )

    pico_set_program_name(AmiCDBox32 "AmiCDBox32")
    pico_set_program_version(AmiCDBox32 "0.3.0")

    pico_enable_stdio_usb(AmiCDBox32 1)
    pico_enable_stdio_uart(AmiCDBox32 0)

    pico_generate_pio_header(AmiCDBox32 ${CMAKE_CURRENT_LIST_DIR}/src/pico.ide/ide_bus.pio)


    # Generate the .uf2 file automatically
    pico_add_extra_outputs(AmiCDBox32)
else()
    message(STATUS "Configuring toolchain for PC Build...")

    project(AmiCDBox32Project C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)    
        
    add_executable(AmiCDBox32
        src/pc/main.PC.cpp
        src/pc/FileSystem.PC.cpp 
        src/pc/ISO9660Parser.cpp
        src/pc/ISO9660Parser.hpp
        src/pc/DeviceController.PC.cpp
#        src/AudioPlayer.cpp
#        src/AudioPlayer.hpp
#        src/BinFile.cpp
#        src/BinFile.hpp
        src/CdDrive.cpp
        src/CdDrive.hpp
        src/Disc.cpp
        src/Disc.hpp
        src/CueParser.cpp
        src/CueParser.hpp
        src/DeviceController.hpp
        src/Endianess.hpp
        src/FileSystem.hpp
        src/IAudioPlayer.hpp
        src/Mounter.cpp
        src/Mounter.hpp
        src/Sector.hpp
        src/Version.hpp
        src/Track.cpp
        src/Track.hpp
        src/VirtualNavDisc.cpp
        src/VirtualNavDisc.hpp
    )

    target_compile_definitions(AmiCDBox32 PRIVATE TARGET_HOST=1)
endif()
