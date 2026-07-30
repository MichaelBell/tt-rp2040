cmake_minimum_required(VERSION 3.12)

# Change your executable name to something creative!
set(NAME etr-hstx) # <-- Name your project/executable here!

set(PICO_PLATFORM rp2350)
set(PICO_BOARD pimoroni_pga2350)

include(pico_sdk_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall -Werror)

# Add your source files
add_executable(${NAME}
    main.c # <-- Add source files here!
    tt_setup.c
    hstx_xfer.c
    8b10b.c
)

pico_generate_pio_header(${NAME} ${CMAKE_CURRENT_LIST_DIR}/recv.pio)

# Don't forget to link the libraries you need!
target_link_libraries(${NAME}
    pico_stdlib
    hardware_gpio
    hardware_pio
    hardware_dma
    pico_multicore
)

pico_enable_stdio_uart(${NAME} 0)
pico_enable_stdio_usb(${NAME} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
