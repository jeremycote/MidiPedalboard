set(NAME MidiController)

add_executable(${NAME}
    main.c
    ads1x15/ads1115.c
)

target_include_directories(${NAME} PUBLIC
    ../inc
    ads1x15
    ../lib/pico-ssd1306/include
)

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
