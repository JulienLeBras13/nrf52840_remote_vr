cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf52840_remote_vr)

target_sources(app PRIVATE
    src/main.c
    src/buttons_service.c
    src/buttons.c
)

# NORDIC SDK APP END
zephyr_library_include_directories(.)
