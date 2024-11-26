pico_sdk_init()

add_subdirectory(shell)
add_subdirectory(commands)

# main
add_executable(
        pico_beken
        main.cpp
        handlers.cpp
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
