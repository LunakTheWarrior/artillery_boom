# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# Raspberry Pi Pico VS Code Extension configuration
# ============================================================

if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)

set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)

if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()

# Pico 2 W
set(PICO_BOARD pico2_w CACHE STRING "Board type")

set(PICO_SDK_FETCH_FROM_GIT 1)

# ============================================================
# Pico SDK
# ============================================================

include(pico_sdk_import.cmake)

project(light_turret C CXX ASM)

pico_sdk_init()

# ============================================================
# Executable
# ============================================================

add_executable(light_turret
    light_turret.cpp
    ModulinoLight.cpp
)

# ============================================================
# Program information
# ============================================================

pico_set_program_name(light_turret "light_turret")
pico_set_program_version(light_turret "0.1")

# ============================================================
# Serial output
# ============================================================

pico_enable_stdio_uart(light_turret 0)
pico_enable_stdio_usb(light_turret 1)

# ============================================================
# Libraries
# ============================================================

target_link_libraries(light_turret
    pico_stdlib
    hardware_i2c
    hardware_pwm
    hardware_gpio
)

# ============================================================
# Include files
# ============================================================

target_include_directories(light_turret PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# ============================================================
# Generate UF2/ELF/etc.
# ============================================================

pico_add_extra_outputs(light_turret)
