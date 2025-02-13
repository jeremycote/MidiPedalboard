cmake_minimum_required(VERSION 3.27)

set(ENV{PICO_SDK_PATH} ${CMAKE_SOURCE_DIR}/pico-sdk)
set(ENV{FREERTOS_KERNEL_PATH} ${CMAKE_SOURCE_DIR}/FreeRTOS-Kernel)

# Include the Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Include the FreeRTOS SDK
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(MidiController C CXX ASM)

# Must initialize Pico SDK before creating executable
pico_sdk_init()

include_directories(
        inc
)

file(GLOB SOURCES
        src/*.c
)

add_executable(MidiController ${SOURCES})

# Link against the SDK
target_link_libraries(MidiController
        pico_stdlib
        hardware_i2c
        hardware_gpio
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4
)

# Enable printf stdout to USB
pico_enable_stdio_usb(MidiController 1)

# Enable printf stdout to UART pins 0 and 1
pico_enable_stdio_uart(MidiController 1)

# Enable the SDK configuration
pico_add_extra_outputs(MidiController)