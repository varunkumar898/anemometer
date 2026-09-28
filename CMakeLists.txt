cmake_minimum_required(VERSION 3.15)
project(baremetal_anemometer C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

# Common core sources
set(CORE_SOURCES
    src/main.c
    src/anemometer.c
    src/mcp3426.c
    src/max7219.c
    src/calibration.c
    src/cli.c
)

# Host Simulation Executable
add_executable(anemometer_sim
    ${CORE_SOURCES}
    hal/hal_sim.c
)

# If cross-compiling with arm-none-eabi-gcc
if(CMAKE_C_COMPILER_ID MATCHES "GNU" AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    add_executable(anemometer_arm.elf
        ${CORE_SOURCES}
        hal/hal_arm_cortex_m.c
        bsp/startup_arm_cm.c
    )
    target_link_options(anemometer_arm.elf PRIVATE
        -T${CMAKE_CURRENT_SOURCE_DIR}/bsp/linker_arm_cm.ld
        -mcpu=cortex-m3
        -mthumb
        -Wl,--gc-sections
        --specs=nano.specs
        --specs=nosys.specs
    )
endif()
