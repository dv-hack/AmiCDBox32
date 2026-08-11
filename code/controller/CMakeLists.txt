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
    
    add_executable(AmiCDBox32
        src/pico/main.PICO.cpp
        src/pico/AudioPlayer.cpp
        src/pico/AudioPlayer.hpp
#        src/pico/DeviceControllerGPIO.cpp
#        src/pico/DeviceControllerGPIO.hpp
        src/pico/DeviceControllerUSB.cpp
        src/pico/DeviceControllerUSB.hpp
        src/pico/FileSystem.PICO.cpp
        src/pico/LedDebugger.cpp
        src/pico/LedDebugger.hpp
        src/pico/PhysicalButton.cpp
        src/pico/PhysicalButton.hpp
        src/pico/PwmBuzzerPlayer.cpp
        src/pico/PwmBuzzerPlayer.hpp
        src/pico/tusb_config.h
        src/pico/usb_descriptors.c
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
        hardware_pwm
        FatFs_SPI
        tinyusb_device
        tinyusb_board
    )

    pico_set_program_name(AmiCDBox32 "AmiCDBox32")
    pico_set_program_version(AmiCDBox32 "0.3.0")

    pico_enable_stdio_usb(AmiCDBox32 0)
    pico_enable_stdio_uart(AmiCDBox32 1)

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
