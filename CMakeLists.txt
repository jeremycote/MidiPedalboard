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

file(GLOB SOURCES
        src/*.c
)

add_executable(MidiController ${SOURCES})

target_include_directories(MidiController PUBLIC
        inc
        ${PICO_SSD1306_DIR}/include
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
        hardware_adc
        FreeRTOS-Kernel-Heap4
        pico_ssd1306
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

# Set up files for the release packages
install(CODE "execute_process(COMMAND $ENV{HOME}/bin/picoDeploy.sh ${CMAKE_CURRENT_BINARY_DIR}/MidiController.elf)")
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/MidiController.uf2
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)

# Set up CPack
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)