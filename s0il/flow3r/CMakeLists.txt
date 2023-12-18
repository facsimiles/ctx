cmake_minimum_required(VERSION 3.12)

set(IDF_TARGET esp32s3)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(s0il)

# Make `1.1` literals into single-point precision float instead of double
# precision. Also warn any time something gets promoted to a double. We
# generally don't want doubles, doubles on ESP32 bad.
idf_component_get_property(flow3r_bsp_lib flow3r_bsp COMPONENT_LIB)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(${flow3r_bsp_lib} PRIVATE -cl-single-precision-constant -Wdouble-promotion)
else()
    target_compile_options(${flow3r_bsp_lib} PRIVATE -fsingle-precision-constant -Wdouble-promotion)
endif()
