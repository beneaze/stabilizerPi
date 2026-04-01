cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(stabilizerpi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(stabilizerpi
    src/main.c
    src/pid.c
)

target_include_directories(stabilizerpi PRIVATE src)

target_link_libraries(stabilizerpi
    pico_stdlib
    hardware_adc
    hardware_pwm
)

# Route stdio to USB (so printf goes over the USB serial port)
pico_enable_stdio_usb(stabilizerpi 1)
pico_enable_stdio_uart(stabilizerpi 0)

pico_add_extra_outputs(stabilizerpi)
