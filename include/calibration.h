#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CALIBRATION_MAGIC_16BIT  0xA6
#define CALIBRATION_STORAGE_SIZE 128

/* Direct byte offsets matching original EEPROM map */
#define ADDR_BRIGHTNESS         0
#define ADDR_WIND_LIMIT         4
#define ADDR_LIMIT_ENABLED      8
#define ADDR_BUZZER_ENABLED     9
#define ADDR_CUTOFF2_ENABLED    10
#define ADDR_MAX_WIND_SENSOR    11
#define ADDR_CUTOFF1_ENABLED    15
#define ADDR_WIFI_TIMEOUT       20
#define ADDR_ADC_MAX            24
#define ADDR_SHUNT_OHMS         28
#define ADDR_ZERO_COUNTS        32
#define ADDR_FS_COUNTS          36
#define ADDR_BUZZER_LIMIT       40
#define ADDR_BUZZER_PERIOD_MS   44
#define ADDR_VERSION_MAGIC      60

typedef struct {
    uint8_t  brightness;        /* 0 .. 15 */
    float    wind_limit;        /* m/s */
    bool     limit_enabled;     /* Master alarm flag */
    bool     buzzer_enabled;    /* Buzzer output enable */
    bool     cutoff1_enabled;   /* Cutoff relay 1 enable */
    bool     cutoff2_enabled;   /* Cutoff relay 2 enable */
    float    max_wind_sensor;   /* Max rated wind speed (m/s) */
    int32_t  adc_max;           /* ADC count ceiling (e.g. 32767) */
    float    shunt_ohms;        /* Sense resistor (e.g. 100.0 ohm) */
    int32_t  zero_counts;       /* Zero wind baseline counts (e.g. 6400) */
    int32_t  fs_counts;         /* Full-scale wind counts (e.g. 32000) */
    float    buzzer_limit;      /* Independent buzzer trigger wind limit (m/s) */
    uint16_t buzzer_period_ms;  /* Buzzer pulse period in ms (e.g. 250, 500, 1000, 0=solid) */
    uint8_t  version_magic;     /* 0xA6 indicates 16-bit calibration */
} calibration_data_t;

extern calibration_data_t g_cal;

void calibration_set_defaults(calibration_data_t *cal);
bool calibration_load(calibration_data_t *cal);
bool calibration_save(const calibration_data_t *cal);

#ifdef __cplusplus
}
#endif

#endif /* CALIBRATION_H */
