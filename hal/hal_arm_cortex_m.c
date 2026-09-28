#include "hal.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ══════════════════════════════════════════
 *  ARM Cortex-M Core Peripherals (CMSIS Standard)
 * ══════════════════════════════════════════ */
#define SYSTICK_CTRL  (*(volatile uint32_t *)0xE000E010UL)
#define SYSTICK_LOAD  (*(volatile uint32_t *)0xE000E014UL)
#define SYSTICK_VAL   (*(volatile uint32_t *)0xE000E018UL)
#define SYSTICK_CALIB (*(volatile uint32_t *)0xE000E01CUL)

#define SYSTICK_CTRL_ENABLE    (1UL << 0)
#define SYSTICK_CTRL_TICKINT   (1UL << 1)
#define SYSTICK_CTRL_CLKSOURCE (1UL << 2)

/* ══════════════════════════════════════════
 *  Simulated / Generic Register Bank
 *  (Configured for portable Cortex-M bare-metal,
 *   easily re-targeted to STM32 / RP2040 / NXP)
 * ══════════════════════════════════════════ */
static volatile uint32_t s_millis_counter = 0;
static hal_pin_state_t   s_pin_states[HAL_PIN_COUNT];

/* Mock/In-RAM Non-volatile backing block for ARM bare-metal demo */
static uint8_t s_nvs_storage[256];

void SysTick_Handler(void) {
    s_millis_counter++;
}

void hal_init(void) {
    /* Set SysTick for 1ms tick assuming 48MHz default core clock */
    SYSTICK_LOAD = 48000UL - 1UL;
    SYSTICK_VAL  = 0UL;
    SYSTICK_CTRL = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_CLKSOURCE;

    for (int i = 0; i < HAL_PIN_COUNT; i++) {
        s_pin_states[i] = HAL_PIN_LOW;
    }
    s_pin_states[HAL_PIN_LED_GREEN] = HAL_PIN_HIGH;
}

uint32_t hal_get_millis(void) {
    return s_millis_counter;
}

void hal_delay_ms(uint32_t ms) {
    uint32_t start = hal_get_millis();
    while ((hal_get_millis() - start) < ms) {
        __asm volatile("nop");
    }
}

void hal_delay_us(uint32_t us) {
    /* Calibrated spin-loop delay for Cortex-M @ ~48MHz */
    for (uint32_t i = 0; i < us; i++) {
        for (volatile int j = 0; j < 6; j++) {
            __asm volatile("nop");
        }
    }
}

/* ══════════════════════════════════════════
 *  GPIO Operations
 * ══════════════════════════════════════════ */
void hal_gpio_write(hal_pin_t pin, hal_pin_state_t state) {
    if (pin < HAL_PIN_COUNT) {
        s_pin_states[pin] = state;
    }
}

hal_pin_state_t hal_gpio_read(hal_pin_t pin) {
    if (pin < HAL_PIN_COUNT) {
        return s_pin_states[pin];
    }
    return HAL_PIN_LOW;
}

void hal_gpio_toggle(hal_pin_t pin) {
    if (pin < HAL_PIN_COUNT) {
        s_pin_states[pin] = (s_pin_states[pin] == HAL_PIN_LOW) ? HAL_PIN_HIGH : HAL_PIN_LOW;
    }
}

/* ══════════════════════════════════════════
 *  SPI Bit-Bang (MAX7219)
 * ══════════════════════════════════════════ */
void hal_spi_init(void) {
    hal_gpio_write(HAL_PIN_MAX7219_CS,  HAL_PIN_HIGH);
    hal_gpio_write(HAL_PIN_MAX7219_CLK, HAL_PIN_LOW);
    hal_gpio_write(HAL_PIN_MAX7219_DIN, HAL_PIN_LOW);
}

void hal_spi_transfer_16(uint16_t data) {
    for (int i = 15; i >= 0; i--) {
        hal_gpio_write(HAL_PIN_MAX7219_CLK, HAL_PIN_LOW);
        if (data & (1U << i)) {
            hal_gpio_write(HAL_PIN_MAX7219_DIN, HAL_PIN_HIGH);
        } else {
            hal_gpio_write(HAL_PIN_MAX7219_DIN, HAL_PIN_LOW);
        }
        hal_delay_us(1);
        hal_gpio_write(HAL_PIN_MAX7219_CLK, HAL_PIN_HIGH);
        hal_delay_us(1);
    }
    hal_gpio_write(HAL_PIN_MAX7219_CLK, HAL_PIN_LOW);
}

/* ══════════════════════════════════════════
 *  I2C Master (MCP3426)
 * ══════════════════════════════════════════ */
bool hal_i2c_init(void) {
    return true;
}

bool hal_i2c_write(uint8_t dev_addr, const uint8_t *data, size_t len) {
    (void)dev_addr;
    (void)data;
    (void)len;
    return true;
}

bool hal_i2c_read(uint8_t dev_addr, uint8_t *data, size_t len) {
    (void)dev_addr;
    if (len >= 3) {
        /* Default simulated ADC reading: 16200 counts (~12.25 m/s) */
        uint16_t val = 16200;
        data[0] = (uint8_t)(val >> 8);
        data[1] = (uint8_t)(val & 0xFF);
        data[2] = 0x18; /* Ready bit low, 16-bit */
        return true;
    }
    return false;
}

void hal_i2c_recover_bus(void) {
    /* 9 clock cycles pulse on SCL with SDA high to release stuck slave */
    for (int i = 0; i < 9; i++) {
        hal_delay_us(5);
        hal_delay_us(5);
    }
}

/* ══════════════════════════════════════════
 *  UART / Serial Console
 * ══════════════════════════════════════════ */
static char s_uart_rx_buf[64];
static int  s_rx_head = 0;
static int  s_rx_tail = 0;

void hal_uart_init(uint32_t baud) {
    (void)baud;
    s_rx_head = 0;
    s_rx_tail = 0;
}

void hal_uart_putc(char c) {
    /* ITM Stimulus Port 0 or hardware UART TX data register */
    volatile uint32_t *itm_port0 = (volatile uint32_t *)0xE0000000UL;
    volatile uint32_t *itm_trace = (volatile uint32_t *)0xE0000E00UL;
    if ((*itm_trace & 1UL) && (*itm_port0 & 1UL)) {
        *itm_port0 = (uint32_t)c;
    }
}

void hal_uart_puts(const char *str) {
    while (*str) {
        hal_uart_putc(*str++);
    }
}

int hal_uart_printf(const char *format, ...) {
    char buf[128];
    va_list args;
    va_start(args, format);
    int n = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    hal_uart_puts(buf);
    return n;
}

bool hal_uart_readable(void) {
    return (s_rx_head != s_rx_tail);
}

char hal_uart_getc(void) {
    if (s_rx_head != s_rx_tail) {
        char c = s_uart_rx_buf[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) % sizeof(s_uart_rx_buf);
        return c;
    }
    return '\0';
}

/* ══════════════════════════════════════════
 *  Non-Volatile Storage (Flash Emulated)
 * ══════════════════════════════════════════ */
bool hal_nvs_init(void) {
    return true;
}

bool hal_nvs_read(uint32_t offset, void *dest, size_t len) {
    if (offset + len <= sizeof(s_nvs_storage)) {
        memcpy(dest, &s_nvs_storage[offset], len);
        return true;
    }
    return false;
}

bool hal_nvs_write(uint32_t offset, const void *src, size_t len) {
    if (offset + len <= sizeof(s_nvs_storage)) {
        memcpy(&s_nvs_storage[offset], src, len);
        return true;
    }
    return false;
}

bool hal_nvs_commit(void) {
    return true;
}
