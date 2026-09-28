#include "max7219.h"
#include "hal.h"

static const uint8_t s_digit_font[10] = {
    0x7E, /* 0: A B C D E F */
    0x30, /* 1: B C */
    0x6D, /* 2: A B D E G */
    0x79, /* 3: A B C D G */
    0x33, /* 4: B C F G */
    0x5B, /* 5: A C D F G */
    0x5F, /* 6: A C D E F G */
    0x70, /* 7: A B C */
    0x7F, /* 8: A B C D E F G */
    0x7B  /* 9: A B C D F G */
};

static uint8_t  s_current_brightness = 8;
static uint32_t s_last_update_ms     = 0;

static void max7219_write_reg(uint8_t reg, uint8_t data) {
    hal_gpio_write(HAL_PIN_MAX7219_CS, HAL_PIN_LOW);
    hal_spi_transfer_16(((uint16_t)reg << 8) | data);
    hal_gpio_write(HAL_PIN_MAX7219_CS, HAL_PIN_HIGH);
}

void max7219_self_heal(void) {
    max7219_write_reg(MAX7219_REG_DISPLAYTEST, 0x00);
    max7219_write_reg(MAX7219_REG_SHUTDOWN,    0x01);
    max7219_write_reg(MAX7219_REG_DECODEMODE,  0x00);
    max7219_write_reg(MAX7219_REG_SCANLIMIT,   MAX7219_NUM_DIGITS - 1);
    max7219_write_reg(MAX7219_REG_INTENSITY,   s_current_brightness & 0x0F);
}

void max7219_set_brightness(uint8_t b) {
    if (b > 15) b = 15;
    s_current_brightness = b;
    max7219_write_reg(MAX7219_REG_INTENSITY, b);
}

void max7219_set_digit(int pos, int digit, bool dp) {
    if (pos < 0 || pos >= MAX7219_NUM_DIGITS) return;
    if (digit < 0 || digit > 9) return;
    uint8_t segs = s_digit_font[digit];
    if (dp) segs |= 0x80;
    max7219_write_reg(MAX7219_REG_DIGIT0 + pos, segs);
}

void max7219_set_raw(int pos, uint8_t segments) {
    if (pos < 0 || pos >= MAX7219_NUM_DIGITS) return;
    max7219_write_reg(MAX7219_REG_DIGIT0 + pos, segments);
}

void max7219_set_blank(int pos) {
    if (pos < 0 || pos >= MAX7219_NUM_DIGITS) return;
    max7219_write_reg(MAX7219_REG_DIGIT0 + pos, 0x00);
}

void max7219_clear(void) {
    uint32_t now = hal_get_millis();
    if (now - s_last_update_ms < MAX7219_UPDATE_INTERVAL_MS) return;
    s_last_update_ms = now;

    max7219_self_heal();
    for (int i = 0; i < MAX7219_NUM_DIGITS; i++) {
        max7219_set_blank(i);
    }
}

void max7219_display_con(void) {
    uint32_t now = hal_get_millis();
    if (now - s_last_update_ms < MAX7219_UPDATE_INTERVAL_MS) return;
    s_last_update_ms = now;

    max7219_self_heal();
    max7219_set_raw(0, 0x4E); /* C */
    max7219_set_raw(1, 0x7E); /* O */
    max7219_set_raw(2, 0x76); /* N */
}

void max7219_display_wid(void) {
    uint32_t now = hal_get_millis();
    if (now - s_last_update_ms < MAX7219_UPDATE_INTERVAL_MS) return;
    s_last_update_ms = now;

    max7219_self_heal();
    max7219_set_raw(0, 0x3E); /* V */
    max7219_set_raw(1, 0x30); /* i */
    max7219_set_raw(2, 0x3D); /* d */
}

void max7219_update_wind(float value) {
    uint32_t now = hal_get_millis();
    if (now - s_last_update_ms < MAX7219_UPDATE_INTERVAL_MS) return;
    s_last_update_ms = now;

    max7219_self_heal();

    if (value < 0.0f)   value = 0.0f;
    if (value > 99.9f)  value = 99.9f;

    int int_val = (int)(value * 10.0f + 0.5f);
    int tens   = (int_val / 100) % 10;
    int ones   = (int_val / 10)  % 10;
    int tenths =  int_val        % 10;

    /* Leading zero suppression for tens digit */
    if (tens == 0 && int_val < 100) {
        max7219_set_blank(0);
    } else {
        max7219_set_digit(0, tens, false);
    }

    max7219_set_digit(1, ones, true); /* Decimal point on digit 1 */
    max7219_set_digit(2, tenths, false);
}

void max7219_init(uint8_t brightness) {
    hal_spi_init();
    hal_gpio_write(HAL_PIN_MAX7219_CS, HAL_PIN_HIGH);

    s_current_brightness = brightness;
    max7219_self_heal();

    for (int i = 0; i < MAX7219_NUM_DIGITS; i++) {
        max7219_set_blank(i);
    }
    s_last_update_ms = 0;
    hal_uart_printf(">>> MAX7219 7-Segment Display Initialized (%d digits)\r\n", MAX7219_NUM_DIGITS);
}
