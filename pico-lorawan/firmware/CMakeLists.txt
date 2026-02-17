cmake_minimum_required(VERSION 3.12)

add_subdirectory(lib/Config)
add_subdirectory(lib/Gas_Sensor)

include_directories(./lib/Config)
include_directories(./lib/Gas_Sensor)


add_executable(lorawan_gas_monitor
    main.c
)

target_link_libraries(lorawan_gas_monitor pico_lorawan hardware_adc Gas_Sensor Config pico_stdlib hardware_adc pico_stdlib)


# enable usb output, disable uart output
pico_enable_stdio_usb(lorawan_gas_monitor 1)
pico_enable_stdio_uart(lorawan_gas_monitor 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(lorawan_gas_monitor)
