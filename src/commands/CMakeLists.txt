add_library(commands INTERFACE)

file(GLOB COMMANDS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

target_sources(
        commands INTERFACE
        ${COMMANDS}
)

target_link_libraries(
        commands INTERFACE
        pico_unique_id
        hardware_clocks
        hardware_adc
        hardware_i2c
        hardware_spi
)
