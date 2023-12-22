include_directories(SYSTEM /usr/include/newlib/c++/12.2.1/arm-none-eabi/)
include_directories(SYSTEM /usr/include/newlib/c++/12.2.1/)

cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(s0il C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(s0il
        main.c
        ctx.h
        ctx.c
        s0il.c
        bundled-programs-arm.c
        )

target_include_directories(s0il PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/.. ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/../../fonts)
# pull in common dependencies
target_link_libraries(s0il
                      pico_stdlib
                      hardware_pio
                      )

    pico_enable_stdio_usb(s0il 1)
    pico_enable_stdio_uart(s0il 0)


# create map/bin/hex file etc.
pico_add_extra_outputs(s0il)


