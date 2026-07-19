cmake_minimum_required(VERSION 3.13)

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

pico_sdk_init()

add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFS_SPI)

add_executable(AmiCDBox32
    src/main.cpp
    src/AudioPlayer.cpp
    src/AudioPlayer.hpp
    src/BinFile.cpp
    src/BinFile.hpp
    src/CdDrive.cpp
    src/CdDrive.hpp
    src/CdMap.cpp
    src/CdMap.hpp
    src/CueParser.cpp
    src/CueParser.hpp
    src/DeviceController.cpp
    src/DeviceController.hpp
    src/IAudioPlayer.hpp
    src/PwmBuzzerPlayer.cpp
    src/PwmBuzzerPlayer.hpp
    src/Sector.hpp
)

# Set C++20 standard requirements
target_compile_features(AmiCDBox32 PRIVATE cxx_std_20)

# Link the standard Pico libraries (GPIO and Standard I/O)
target_link_libraries(AmiCDBox32 
pico_stdlib
hardware_spi
hardware_pwm
FatFs_SPI
)

pico_set_program_name(AmiCDBox32 "AmiCDBox32")
pico_set_program_version(AmiCDBox32 "0.3.0")

# Enable USB serial output (instead of hardware UART)
pico_enable_stdio_usb(AmiCDBox32 1)
pico_enable_stdio_uart(AmiCDBox32 0)

# Generate the .uf2 file automatically
pico_add_extra_outputs(AmiCDBox32)