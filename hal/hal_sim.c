#include "hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <unistd.h>
#include <time.h>
#endif

static hal_pin_state_t s_sim_pins[HAL_PIN_COUNT];
static uint8_t         s_sim_nvs[256];
static uint64_t        s_start_time_ms = 0;

static uint64_t get_host_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&counter)) {
        return (uint64_t)((counter.QuadPart * 1000) / freq.QuadPart);
    }
    return (uint64_t)GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

void hal_init(void) {
    s_start_time_ms = get_host_time_ms();
    for (int i = 0; i < HAL_PIN_COUNT; i++) {
        s_sim_pins[i] = HAL_PIN_LOW;
    }
    s_sim_pins[HAL_PIN_LED_GREEN] = HAL_PIN_HIGH;
}

uint32_t hal_get_millis(void) {
    return (uint32_t)(get_host_time_ms() - s_start_time_ms);
}

void hal_delay_ms(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

void hal_delay_us(uint32_t us) {
#ifdef _WIN32
    /* Spin-wait for sub-millisecond precision */
    LARGE_INTEGER freq, start, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    int64_t ticks = (int64_t)((double)us * (double)freq.QuadPart / 1000000.0);
    do {
        QueryPerformanceCounter(&now);
    } while ((now.QuadPart - start.QuadPart) < ticks);
#else
    usleep(us);
#endif
}

void hal_gpio_write(hal_pin_t pin, hal_pin_state_t state) {
    if (pin < HAL_PIN_COUNT) {
        s_sim_pins[pin] = state;
    }
}

hal_pin_state_t hal_gpio_read(hal_pin_t pin) {
    if (pin < HAL_PIN_COUNT) {
        return s_sim_pins[pin];
    }
    return HAL_PIN_LOW;
}

void hal_gpio_toggle(hal_pin_t pin) {
    if (pin < HAL_PIN_COUNT) {
        s_sim_pins[pin] = (s_sim_pins[pin] == HAL_PIN_LOW) ? HAL_PIN_HIGH : HAL_PIN_LOW;
    }
}

/* ══════════════════════════════════════════
 *  SPI Simulation (MAX7219)
 * ══════════════════════════════════════════ */
void hal_spi_init(void) {
    /* Ready */
}

void hal_spi_transfer_16(uint16_t data) {
    (void)data;
    /* Simulated MAX7219 SPI shift */
}

/* ══════════════════════════════════════════
 *  I2C Simulation (MCP3426 ADC)
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
        uint16_t count = 16200;
        data[0] = (uint8_t)(count >> 8);
        data[1] = (uint8_t)(count & 0xFF);
        data[2] = 0x18;
        return true;
    }
    return false;
}

void hal_i2c_recover_bus(void) {
    /* No-op in simulation */
}

/* ══════════════════════════════════════════
 *  UART Simulation
 * ══════════════════════════════════════════ */
void hal_uart_init(uint32_t baud) {
    (void)baud;
}

void hal_uart_putc(char c) {
    putchar(c);
    fflush(stdout);
}

void hal_uart_puts(const char *str) {
    fputs(str, stdout);
    fflush(stdout);
}

int hal_uart_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    fflush(stdout);
    return ret;
}

bool hal_uart_readable(void) {
#ifdef _WIN32
    if (_isatty(_fileno(stdin))) {
        return _kbhit() != 0;
    } else {
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
        DWORD avail = 0;
        if (PeekNamedPipe(h, NULL, 0, NULL, &avail, NULL)) {
            return avail > 0;
        }
        return !feof(stdin);
    }
#else
    return false;
#endif
}

char hal_uart_getc(void) {
#ifdef _WIN32
    if (_isatty(_fileno(stdin))) {
        if (_kbhit()) {
            return (char)_getch();
        }
    } else {
        int c = getchar();
        if (c != EOF) {
            return (char)c;
        }
    }
#endif
    return '\0';
}

/* ══════════════════════════════════════════
 *  Non-Volatile Storage (File-backed Simulation)
 * ══════════════════════════════════════════ */
#define NVS_SIM_FILE "calib_storage.bin"

bool hal_nvs_init(void) {
    FILE *f = fopen(NVS_SIM_FILE, "rb");
    if (f) {
        size_t n = fread(s_sim_nvs, 1, sizeof(s_sim_nvs), f);
        fclose(f);
        if (n == sizeof(s_sim_nvs)) return true;
    }
    return true;
}

bool hal_nvs_read(uint32_t offset, void *dest, size_t len) {
    if (offset + len <= sizeof(s_sim_nvs)) {
        memcpy(dest, &s_sim_nvs[offset], len);
        return true;
    }
    return false;
}

bool hal_nvs_write(uint32_t offset, const void *src, size_t len) {
    if (offset + len <= sizeof(s_sim_nvs)) {
        memcpy(&s_sim_nvs[offset], src, len);
        return true;
    }
    return false;
}

bool hal_nvs_commit(void) {
    FILE *f = fopen(NVS_SIM_FILE, "wb");
    if (f) {
        fwrite(s_sim_nvs, 1, sizeof(s_sim_nvs), f);
        fclose(f);
        return true;
    }
    return false;
}
