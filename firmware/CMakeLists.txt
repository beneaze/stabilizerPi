cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2 CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(stabilizerpi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(stabilizerpi
    src/main.c
    src/pid.c
    src/aom_lut.c
)

target_include_directories(stabilizerpi PRIVATE src)

target_link_libraries(stabilizerpi
    pico_stdlib
    hardware_adc
    hardware_gpio
    hardware_pwm
)

# Route stdio to USB (so printf goes over the USB serial port)
pico_enable_stdio_usb(stabilizerpi 1)
pico_enable_stdio_uart(stabilizerpi 0)

pico_add_extra_outputs(stabilizerpi)

# --- Ramp test (open-loop AOM sweep for hardware bring-up) ---
add_executable(ramp_test
    src/ramp_test.c
)

target_include_directories(ramp_test PRIVATE src)

target_link_libraries(ramp_test
    pico_stdlib
    hardware_adc
    hardware_gpio
    hardware_pwm
)

pico_enable_stdio_usb(ramp_test 1)
pico_enable_stdio_uart(ramp_test 0)

pico_add_extra_outputs(ramp_test)

# --- Ramp test through AOM LUT (matches stabilizerpi drive path) ---
add_executable(ramp_test_lut
    src/ramp_test_lut.c
    src/aom_lut.c
)

target_include_directories(ramp_test_lut PRIVATE src)

target_link_libraries(ramp_test_lut
    pico_stdlib
    hardware_adc
    hardware_gpio
    hardware_pwm
)

pico_enable_stdio_usb(ramp_test_lut 1)
pico_enable_stdio_uart(ramp_test_lut 0)

pico_add_extra_outputs(ramp_test_lut)
