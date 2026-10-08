# Bare-Metal Industrial Anemometer Firmware (16-Bit MCP3426 + MAX7219)

A portable, modular **Bare-Metal C** implementation of the Industrial Wind Monitoring System (MCP3426 16-bit Delta-Sigma I2C ADC + MAX7219 7-Segment Display + Dual Cutoff Interlocks).

This firmware contains featuring a **Hardware Abstraction Layer (HAL)**, direct peripheral drivers, standard C math, and a responsive **Serial UART Command-Line Interface (CLI)**.

---

## 🏗️ Architecture Overview

```
.
├── include/
│   ├── anemometer.h     # Core wind calculations, EMA filter, fault detection, alarms
│   ├── calibration.h    # 16-bit calibration structure & non-volatile storage layout
│   ├── cli.h            # Serial UART interactive command-line interface
│   ├── hal.h            # Hardware Abstraction Layer interface (GPIO, I2C, SPI, UART, Timer)
│   ├── max7219.h        # 7-Segment display driver (3-digit XX.X format, CON/WID glyphs)
│   └── mcp3426.h        # 16-bit Delta-Sigma I2C ADC driver (0x68 continuous mode)
├── src/
│   ├── main.c           # Bare-metal superloop entry point
│   ├── anemometer.c     # Anemometer engine implementation
│   ├── calibration.c    # EEPROM/Flash calibration migration and storage
│   ├── cli.c            # Command processor (data, status, setlimit, setzero, save...)
│   ├── max7219.c        # MAX7219 register driver, font tables, and self-heal
│   └── mcp3426.c        # MCP3426 continuous read and bus-recovery logic
├── hal/
│   ├── hal_arm_cortex_m.c # Hardware implementation for ARM Cortex-M microcontrollers
│   └── hal_sim.c          # Host PC simulation HAL (runs on Windows/Linux via MinGW/GCC)
├── bsp/
│   ├── startup_arm_cm.c # Cortex-M vector table, reset handler, stack initialization
│   └── linker_arm_cm.ld # Flash (256K) and RAM (64K) linker script
├── test/
│   └── test_anemometer.c# Complete automated unit test suite
├── Makefile             # Multi-target build system (ARM, Host, Tests)
├── CMakeLists.txt       # Cross-platform CMake configuration
└── README.md
```

---

## ⚡ Compilation & Targets

The build system supports dual targets out of the box using GNU Make:

### 1. Build ARM Cortex-M Bare-Metal Firmware
Targeting microcontrollers (such as STM32, RP2040, SAMD, or Renode emulation) using `arm-none-eabi-gcc`:
```bash
make arm
```
**Outputs:**
- `build/anemometer_cortex_m.elf`
- `build/anemometer_cortex_m.bin` (ready for flashing via OpenOCD, ST-Link, or J-Link)
- `build/anemometer_cortex_m.hex`

### 2. Build Host Simulation Executable
Compile directly on your PC with MinGW `gcc`:
```bash
make host
```
**Output:**
- `build/anemometer_sim.exe` (Run interactive CLI and simulate sensor inputs on your PC)

### 3. Run Automated Unit Tests
Validates the 4–20 mA transfer function, 16-bit calibration math, EMA convergence, alarm thresholds, deadband clamping, and open-circuit fault detection:
```bash
make test
```

### 4. Build All Targets
```bash
make all
```

### 5. Clean Build Artifacts
```bash
make clean
```

---

## 📟 Interactive Serial UART CLI Commands

Connect to the device over UART at **115200 baud (8N1)**:

| Command | Arguments | Description | Example |
| :--- | :--- | :--- | :--- |
| `help` | none | Displays list of all available commands | `help` |
| `data` | none | Returns live JSON telemetry (matches Arduino `/data` route) | `data` -> `{"wind":"12.45","raw":"16200"}` |
| `status` | none | Detailed ASCII table of sensor, relays, buzzer, and limits | `status` |
| `setlimit` | `<float>` | Sets wind speed alarm threshold ($m/s$) | `setlimit 22.5` |
| `setmax` | `<float>` | Sets maximum rated sensor speed ($m/s$) | `setmax 45.0` |
| `setbright`| `<0-15>` | Adjusts MAX7219 7-segment display intensity | `setbright 10` |
| `setzero` | `<counts>` | Sets 4 mA baseline calibration ADC count | `setzero 6400` |
| `setfs` | `<counts>` | Sets 20 mA full-scale calibration ADC count | `setfs 32000` |
| `setshunt` | `<ohms>` | Sets shunt resistor resistance ($\Omega$) | `setshunt 100.0` |
| `setadcmax`| `<counts>` | Sets max ADC count ceiling | `setadcmax 32767` |
| `alarm` | `<1 \| 0>` | Enables / disables master alarm interlock | `alarm 1` |
| `buzzer` | `<1 \| 0>` | Enables / disables pulsed audio buzzer output | `buzzer 1` |
| `setbuzzlimit` | `<float>` | Sets independent buzzer trigger wind limit ($m/s$) | `setbuzzlimit 18.0` |
| `setbuzzrate`  | `<ms>` | Sets buzzer pulse period in $ms$ (e.g. 250, 500, 1000, 0=solid) | `setbuzzrate 250` |
| `cutoff1` | `<1 \| 0>` | Enables / disables Cutoff Relay 1 | `cutoff1 1` |
| `cutoff2` | `<1 \| 0>` | Enables / disables Cutoff Relay 2 | `cutoff2 1` |
| `save` | none | Commits modified calibration to non-volatile storage | `save` |
| `reset` | none | Restores factory default 16-bit calibration values | `reset` |
| `stream` | `<1 \| 0>` | Toggles continuous 500ms telemetry output | `stream 1` |
| `sim` | `<counts>` | Injects mock ADC count for testing (-1 to revert to hardware) | `sim 20000` |
| `exit` | none | Exits simulation environment (host target only) | `exit` |

---

## 📐 Signal Processing & Math Specification

The transfer function exactly implements the industrial 4–20 mA current loop formula:

$$\text{Span} = \text{FS\_COUNTS} - \text{ZERO\_COUNTS}$$

$$\text{Wind Speed } (m/s) = (\text{ADC}_{\text{filtered}} - \text{ZERO\_COUNTS}) \times \left(\frac{\text{MaxWindSensor}}{\text{Span}}\right)$$

- **Continuous 16-Bit Resolution**: $0$ to $32767$ counts ($62.5\ \mu\text{V/LSB}$ across $100\ \Omega$ shunt).
- **EMA Filter**: $\alpha = 0.2$ ($\text{ADC}_{\text{filtered}} = 0.8 \times \text{History} + 0.2 \times \text{New}$).
- **Deadband**: Clamps $< 0.2\text{ m/s}$ to $0.0\text{ m/s}$.
- **Fault Detection**: Signals $< 100$ counts (with Zero $> 500$) indicate open circuit / cable break.
- **Power-On Stabilization**: 4000ms grace period suppresses false alarms while power rails stabilize.

---

## 🔌 Hardware Porting Guide (HAL)

To adapt the firmware to a specific board or microcontroller:
1. Open `hal/hal_arm_cortex_m.c` (or add a new `hal_<target>.c`).
2. Map your MCU's GPIO registers in:
   - `hal_gpio_write(hal_pin_t pin, hal_pin_state_t state)`
   - `hal_gpio_read(hal_pin_t pin)`
3. Map your hardware I2C or use the provided portable bit-bang I2C in `hal_i2c_*`.
4. Connect the UART peripheral TX/RX interrupts to `hal_uart_*`.
5. Point the linker script `bsp/linker_arm_cm.ld` to your microcontroller's Flash and SRAM size.
