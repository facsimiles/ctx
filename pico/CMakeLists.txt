cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(ctxterm C CXX ASM)
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

add_executable(ctxterm
        term.c
        ctx.h
        ctx.c
        tusb_keyboard.c
        tusb_config.h
        )

target_include_directories(ctxterm PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# pull in common dependencies
target_link_libraries(ctxterm
                      pico_stdlib
                      hardware_pio
                      hardware_uart
                      tinyusb_host
                      tinyusb_board)

# create map/bin/hex file etc.
pico_add_extra_outputs(ctxterm)

