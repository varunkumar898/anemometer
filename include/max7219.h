#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX7219_NUM_DIGITS        3
#define MAX7219_UPDATE_INTERVAL_MS 200

#define MAX7219_REG_DIGIT0        0x01
#define MAX7219_REG_DIGIT1        0x02
#define MAX7219_REG_DIGIT2        0x03
#define MAX7219_REG_DECODEMODE    0x09
#define MAX7219_REG_INTENSITY     0x0A
#define MAX7219_REG_SCANLIMIT     0x0B
#define MAX7219_REG_SHUTDOWN      0x0C
#define MAX7219_REG_DISPLAYTEST   0x0F

void max7219_init(uint8_t brightness);
void max7219_set_brightness(uint8_t b);
void max7219_update_wind(float wind_speed);
void max7219_display_con(void);
void max7219_display_wid(void);
void max7219_clear(void);
void max7219_self_heal(void);
void max7219_set_digit(int pos, int digit, bool dp);
void max7219_set_raw(int pos, uint8_t segments);
void max7219_set_blank(int pos);

#ifdef __cplusplus
}
#endif

#endif /* MAX7219_H */
