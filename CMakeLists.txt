cmake_minimum_required(VERSION 3.13)

set(PICO_PLATFORM "rp2040")
set(PICO_BOARD "pico")

include(pico_sdk_import.cmake)
include(scripts/functions.cmake)

project(pico-beken C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(src/shell)
add_subdirectory(src/commands)

# main
add_executable(
        pico_beken
        src/main.cpp
        src/handlers.cpp
)

pico_enable_stdio_usb(pico_beken 1)
pico_enable_stdio_uart(pico_beken 0)

pico_add_extra_outputs(pico_beken)

target_link_libraries(
        pico_beken
        pico_stdlib
        shell
        commands
)

add_deploy_target(pico_beken "${CMAKE_CURRENT_BINARY_DIR}/pico_beken.uf2")
add_custom_target(
        flash
        DEPENDS pico_beken---deploy
)