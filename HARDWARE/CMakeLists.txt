add_library(hardware_drivers STATIC
    SRC/ad7616.c
    SRC/ad9833.c
    SRC/ad9834.c
    SRC/Communication.c
    SRC/delay.c
    SRC/lcd.c
)

target_include_directories(hardware_drivers
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/INC
)

target_link_libraries(hardware_drivers
    PUBLIC
        stm32cubemx
        m
)