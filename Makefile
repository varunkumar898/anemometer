# ══════════════════════════════════════════════════════════════════
#  Bare-Metal Industrial Anemometer Firmware Makefile
# ══════════════════════════════════════════════════════════════════

# Toolchains
ARM_CC      ?= arm-none-eabi-gcc
ARM_OBJCOPY ?= arm-none-eabi-objcopy
ARM_SIZE    ?= arm-none-eabi-size

HOST_CC     ?= gcc

# Cross-platform commands
ifeq ($(OS),Windows_NT)
    FIXPATH = $(subst /,\,$(1))
    MKDIR   = @if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    CLEAN_BUILD = @if exist build rmdir /s /q build
    CLEAN_NVS   = @if exist calib_storage.bin del /f /q calib_storage.bin
else
    FIXPATH = $(1)
    MKDIR   = @mkdir -p $(1)
    CLEAN_BUILD = @rm -rf build
    CLEAN_NVS   = @rm -f calib_storage.bin
endif

# Directories
BUILD_DIR   := build
SRC_DIR     := src
INC_DIR     := include
HAL_DIR     := hal
BSP_DIR     := bsp

# Common Core Sources
CORE_SRCS   := $(SRC_DIR)/main.c \
               $(SRC_DIR)/anemometer.c \
               $(SRC_DIR)/mcp3426.c \
               $(SRC_DIR)/max7219.c \
               $(SRC_DIR)/calibration.c \
               $(SRC_DIR)/cli.c

# Target 1: ARM Cortex-M Bare-Metal
ARM_SRCS    := $(CORE_SRCS) \
               $(HAL_DIR)/hal_arm_cortex_m.c \
               $(BSP_DIR)/startup_arm_cm.c

ARM_OBJS    := $(patsubst %.c, $(BUILD_DIR)/arm/%.o, $(ARM_SRCS))

ARM_CFLAGS  := -mcpu=cortex-m3 -mthumb -Wall -Wextra -O2 \
               -I$(INC_DIR) -ffunction-sections -fdata-sections

ARM_LDFLAGS := -mcpu=cortex-m3 -mthumb -T $(BSP_DIR)/linker_arm_cm.ld \
               -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs \
               -lc -lm -lnosys

ARM_ELF     := $(BUILD_DIR)/anemometer_cortex_m.elf
ARM_BIN     := $(BUILD_DIR)/anemometer_cortex_m.bin
ARM_HEX     := $(BUILD_DIR)/anemometer_cortex_m.hex

# Target 2: Host Simulation (Windows / MinGW / Linux)
HOST_SRCS   := $(CORE_SRCS) \
               $(HAL_DIR)/hal_sim.c

HOST_OBJS   := $(patsubst %.c, $(BUILD_DIR)/host/%.o, $(HOST_SRCS))
HOST_CFLAGS := -Wall -Wextra -O2 -I$(INC_DIR)
HOST_BIN    := $(BUILD_DIR)/anemometer_sim.exe

# Target 3: Unit Tests
TEST_SRCS   := test/test_anemometer.c \
               src/anemometer.c \
               src/mcp3426.c \
               src/max7219.c \
               src/calibration.c \
               src/cli.c \
               hal/hal_sim.c
TEST_BIN    := $(BUILD_DIR)/test_anemometer.exe

.PHONY: all arm host test clean help

all: arm host test

help:
	@echo Industrial Anemometer Bare-Metal Build System
	@echo Targets:
	@echo   make arm   - Build bare-metal ARM Cortex-M ELF, BIN, HEX using arm-none-eabi-gcc
	@echo   make host  - Build host simulation EXE using gcc
	@echo   make all   - Build both ARM and Host targets
	@echo   make clean - Remove build artifacts

arm: $(ARM_BIN) $(ARM_HEX)
	@echo --------------------------------------------------------
	@echo ARM Cortex-M Bare-Metal Build Complete:
	@echo   ELF: $(ARM_ELF)
	@echo   BIN: $(ARM_BIN)
	@echo   HEX: $(ARM_HEX)
	@echo --------------------------------------------------------
	$(ARM_SIZE) $(ARM_ELF)

$(ARM_BIN): $(ARM_ELF)
	$(ARM_OBJCOPY) -O binary $< $@

$(ARM_HEX): $(ARM_ELF)
	$(ARM_OBJCOPY) -O ihex $< $@

$(ARM_ELF): $(ARM_OBJS)
	$(call MKDIR,$(dir $@))
	$(ARM_CC) $(ARM_OBJS) $(ARM_LDFLAGS) -o $@

$(BUILD_DIR)/arm/%.o: %.c
	$(call MKDIR,$(dir $@))
	$(ARM_CC) $(ARM_CFLAGS) -c $< -o $@

host: $(HOST_BIN)
	@echo --------------------------------------------------------
	@echo Host Simulation Executable Built:
	@echo   Binary: $(HOST_BIN)
	@echo --------------------------------------------------------

$(HOST_BIN): $(HOST_OBJS)
	$(call MKDIR,$(dir $@))
	$(HOST_CC) $(HOST_OBJS) -o $@

$(BUILD_DIR)/host/%.o: %.c
	$(call MKDIR,$(dir $@))
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

test: $(TEST_BIN)
	@echo --------------------------------------------------------
	@echo Running Anemometer Unit Tests...
	@echo --------------------------------------------------------
	@$(TEST_BIN)

$(TEST_BIN): $(TEST_SRCS)
	$(call MKDIR,$(dir $@))
	$(HOST_CC) $(HOST_CFLAGS) $^ -o $@

clean:
	$(CLEAN_BUILD)
	$(CLEAN_NVS)
