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

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Include pico-ssd1306 library
set(PICO_SSD1306_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/pico-ssd1306")
include(pico-ssd1306.cmake)

# Wi-Fi config
if(DEFINED ENV{WIFI_SSID} AND DEFINED ENV{WIFI_PASS})
    message(STATUS "Using Wi-Fi SSID: $ENV{WIFI_SSID}")
    add_definitions(-DWIFI_SSID="$ENV{WIFI_SSID}" -DWIFI_PASS="$ENV{WIFI_PASS}")
else()
    message(FATAL_ERROR "Wi-Fi SSID and/or password environment variables are not set.")
endif()

set(NAME MidiController)

file(GLOB SOURCES src/*.c)

add_executable(${NAME} ${SOURCES})

target_include_directories(${NAME} PUBLIC inc lib/pico-ssd1306/include)

target_link_libraries(${NAME}
        pico_stdlib
        pico_ssd1306
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_mbedtls
        pico_lwip_mdns
        pico_mbedtls
        hardware_i2c
        hardware_gpio
        hardware_adc
        FreeRTOS-Kernel-Heap4
)

pico_add_extra_outputs(${NAME})
pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 1)

target_compile_definitions(${NAME} PRIVATE
        PICO_DEFAULT_UART_RX_PIN=1
        PICO_DEFAULT_UART_TX_PIN=0
)

add_custom_command(TARGET MidiController POST_BUILD
        COMMAND ../pico_install.sh ${CMAKE_CURRENT_BINARY_DIR}/MidiController.uf2
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Flashing Pico after successful build..."
)