cmake_minimum_required(VERSION 3.27)

set(PICO_BOARD pico_w)

set(ENV{PICO_SDK_PATH} ${CMAKE_SOURCE_DIR}/pico-sdk)
set(ENV{FREERTOS_KERNEL_PATH} ${CMAKE_SOURCE_DIR}/FreeRTOS-Kernel)

# Include the Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Include the FreeRTOS SDK
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(MidiController C CXX ASM)

# Must initialize Pico SDK before creating executable
pico_sdk_init()

file(GLOB SOURCES
        src/*.c
)

add_executable(MidiController ${SOURCES})

target_include_directories(MidiController PUBLIC
        inc
)
# Link against the SDK
target_link_libraries(MidiController
        pico_stdlib
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_mbedtls
        pico_lwip_mdns
        pico_mbedtls
        hardware_i2c
        hardware_gpio
        FreeRTOS-Kernel-Heap4
)

# Enable printf stdout to USB
pico_enable_stdio_usb(MidiController 1)

# Enable printf stdout to UART pins 0 and 1
pico_enable_stdio_uart(MidiController 1)

# Enable the SDK configuration
pico_add_extra_outputs(MidiController)

# Get Wi-Fi SSID and password from environment variables and define them
if(DEFINED ENV{WIFI_SSID} AND DEFINED ENV{WIFI_PASS})
    message(STATUS "Using Wi-Fi SSID: $ENV{WIFI_SSID}")
    add_definitions(-DWIFI_SSID="$ENV{WIFI_SSID}" -DWIFI_PASS="$ENV{WIFI_PASS}")
else()
    message(FATAL_ERROR "Wi-Fi SSID and/or password environment variables are not set.")
endif()