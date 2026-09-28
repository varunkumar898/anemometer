#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ══════════════════════════════════════════
 *  GPIO Pin Identifier Enum
 * ══════════════════════════════════════════ */
typedef enum {
    HAL_PIN_BUZZER = 0,
    HAL_PIN_CUTOFF1,
    HAL_PIN_CUTOFF2,
    HAL_PIN_LED_RED,
    HAL_PIN_LED_GREEN,
    HAL_PIN_MAX7219_DIN,
    HAL_PIN_MAX7219_CLK,
    HAL_PIN_MAX7219_CS,
    HAL_PIN_COUNT
} hal_pin_t;

typedef enum {
    HAL_PIN_LOW  = 0,
    HAL_PIN_HIGH = 1
} hal_pin_state_t;

/* ══════════════════════════════════════════
 *  HAL Core Lifecycle & Timing
 * ══════════════════════════════════════════ */
void     hal_init(void);
uint32_t hal_get_millis(void);
void     hal_delay_ms(uint32_t ms);
void     hal_delay_us(uint32_t us);

/* ══════════════════════════════════════════
 *  GPIO Operations
 * ══════════════════════════════════════════ */
void            hal_gpio_write(hal_pin_t pin, hal_pin_state_t state);
hal_pin_state_t hal_gpio_read(hal_pin_t pin);
void            hal_gpio_toggle(hal_pin_t pin);

/* ══════════════════════════════════════════
 *  I2C Master Interface (MCP3426 ADC)
 * ══════════════════════════════════════════ */
bool hal_i2c_init(void);
bool hal_i2c_write(uint8_t dev_addr, const uint8_t *data, size_t len);
bool hal_i2c_read(uint8_t dev_addr, uint8_t *data, size_t len);
void hal_i2c_recover_bus(void);

/* ══════════════════════════════════════════
 *  SPI / Bit-bang Interface (MAX7219)
 * ══════════════════════════════════════════ */
void hal_spi_init(void);
void hal_spi_transfer_16(uint16_t data);

/* ══════════════════════════════════════════
 *  UART / Serial Console Interface
 * ══════════════════════════════════════════ */
void hal_uart_init(uint32_t baud);
void hal_uart_putc(char c);
void hal_uart_puts(const char *str);
int  hal_uart_printf(const char *format, ...);
bool hal_uart_readable(void);
char hal_uart_getc(void);

/* ══════════════════════════════════════════
 *  Non-Volatile Storage (EEPROM / Flash)
 * ══════════════════════════════════════════ */
bool hal_nvs_init(void);
bool hal_nvs_read(uint32_t offset, void *dest, size_t len);
bool hal_nvs_write(uint32_t offset, const void *src, size_t len);
bool hal_nvs_commit(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_H */
