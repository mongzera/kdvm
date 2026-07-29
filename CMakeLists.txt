cmake_minimum_required(VERSION 3.13)

# Include Pico SDK configuration
include(pico_sdk_import.cmake OPTIONAL)
if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH environment variable is not set. Set it or place pico_sdk_import.cmake in the project root.")
endif()

project(grrvm C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Collect all C source files
file(GLOB_RECURSE SOURCES
    "main.c"
    "core/*.c"
    "core/eval/*.c"
    "math/*.c"
    "io/*.c"
    "util/hashmap/*.c"
    "util/types/*.c"
    "memory/*.c"
    "assembler/*.c"
)

# Add executable
add_executable(grrvm ${SOURCES})

# Include project directories
target_include_directories(grrvm PRIVATE .)

# Link standard Pico libraries and math library
target_link_libraries(grrvm
    pico_stdlib
    m
)

# Enable UART output for Wokwi's Serial Monitor (and optionally USB serial)
pico_enable_stdio_uart(grrvm 1)
pico_enable_stdio_usb(grrvm 0)

# Generate extra build outputs (.elf, .hex, .uf2, .map)
pico_add_extra_outputs(grrvm)
