cmake_minimum_required(VERSION 3.13)

# Initialize the SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(AmiCDBox32Project C CXX ASM)

pico_sdk_init()

add_executable(AmiCDBox32
    src/main.cpp
    src/CueParser.cpp
    src/CueParser.hpp
    src/AudioPlayer.cpp
    src/AudioPlayer.hpp
    src/CdDrive.cpp
    src/CdDrive.hpp
    src/DeviceController.cpp
    src/DeviceController.hpp
)
# set C++20 
target_compile_features(AmiCDBox32 PRIVATE cxx_std_20)

# Link the standard Pico libraries (GPIO and Standard I/O)
target_link_libraries(AmiCDBox32 pico_stdlib)

# Enable USB serial output (instead of hardware UART)
pico_enable_stdio_usb(AmiCDBox32 1)
pico_enable_stdio_uart(AmiCDBox32 0)

# Generate the .uf2 file automatically
pico_add_extra_outputs(AmiCDBox32)
