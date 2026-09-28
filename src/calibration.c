#include "calibration.h"
#include "hal.h"
#include <string.h>

calibration_data_t g_cal;

void calibration_set_defaults(calibration_data_t *cal) {
    if (!cal) return;
    cal->brightness       = 8;
    cal->wind_limit       = 20.00f;
    cal->limit_enabled    = false;
    cal->buzzer_enabled   = false;
    cal->cutoff1_enabled  = false;
    cal->cutoff2_enabled  = false;
    cal->max_wind_sensor  = 45.00f;
    cal->adc_max          = 32767;
    cal->shunt_ohms       = 100.0f;
    cal->zero_counts      = 6400;
    cal->fs_counts        = 32000;
    cal->buzzer_limit     = 20.00f;
    cal->buzzer_period_ms = 500;
    cal->version_magic    = CALIBRATION_MAGIC_16BIT;
}

bool calibration_load(calibration_data_t *cal) {
    if (!cal) return false;

    if (!hal_nvs_init()) {
        calibration_set_defaults(cal);
        return false;
    }

    uint8_t magic = 0;
    hal_nvs_read(ADDR_VERSION_MAGIC, &magic, sizeof(magic));

    if (magic != CALIBRATION_MAGIC_16BIT) {
        calibration_set_defaults(cal);
        calibration_save(cal);
        return false;
    }

    uint8_t b = 8;
    hal_nvs_read(ADDR_BRIGHTNESS, &b, sizeof(b));
    cal->brightness = (b <= 15) ? b : 8;

    float wl = 20.0f;
    hal_nvs_read(ADDR_WIND_LIMIT, &wl, sizeof(wl));
    cal->wind_limit = (wl >= 0.0f && wl <= 200.0f) ? wl : 20.0f;

    uint8_t flag = 0;
    hal_nvs_read(ADDR_LIMIT_ENABLED, &flag, sizeof(flag));
    cal->limit_enabled = (flag != 0);

    hal_nvs_read(ADDR_BUZZER_ENABLED, &flag, sizeof(flag));
    cal->buzzer_enabled = (flag != 0);

    hal_nvs_read(ADDR_CUTOFF1_ENABLED, &flag, sizeof(flag));
    cal->cutoff1_enabled = (flag != 0);

    hal_nvs_read(ADDR_CUTOFF2_ENABLED, &flag, sizeof(flag));
    cal->cutoff2_enabled = (flag != 0);

    float mw = 45.0f;
    hal_nvs_read(ADDR_MAX_WIND_SENSOR, &mw, sizeof(mw));
    cal->max_wind_sensor = (mw > 0.0f && mw <= 200.0f) ? mw : 45.0f;

    int32_t adc_max = 32767;
    hal_nvs_read(ADDR_ADC_MAX, &adc_max, sizeof(adc_max));
    cal->adc_max = (adc_max >= 1 && adc_max <= 65535) ? adc_max : 32767;

    float shunt = 100.0f;
    hal_nvs_read(ADDR_SHUNT_OHMS, &shunt, sizeof(shunt));
    cal->shunt_ohms = (shunt >= 0.1f && shunt <= 10000.0f) ? shunt : 100.0f;

    int32_t zero = 6400;
    hal_nvs_read(ADDR_ZERO_COUNTS, &zero, sizeof(zero));
    cal->zero_counts = (zero >= 0 && zero <= 65535) ? zero : 6400;

    int32_t fs = 32000;
    hal_nvs_read(ADDR_FS_COUNTS, &fs, sizeof(fs));
    cal->fs_counts = (fs >= 0 && fs <= 65535) ? fs : 32000;

    float bl = 20.0f;
    hal_nvs_read(ADDR_BUZZER_LIMIT, &bl, sizeof(bl));
    cal->buzzer_limit = (bl >= 0.0f && bl <= 200.0f) ? bl : cal->wind_limit;

    uint16_t bp = 500;
    hal_nvs_read(ADDR_BUZZER_PERIOD_MS, &bp, sizeof(bp));
    cal->buzzer_period_ms = (bp <= 5000) ? bp : 500;

    cal->version_magic = CALIBRATION_MAGIC_16BIT;
    return true;
}

bool calibration_save(const calibration_data_t *cal) {
    if (!cal) return false;

    uint8_t b = cal->brightness;
    hal_nvs_write(ADDR_BRIGHTNESS, &b, sizeof(b));

    float wl = cal->wind_limit;
    hal_nvs_write(ADDR_WIND_LIMIT, &wl, sizeof(wl));

    uint8_t flag = cal->limit_enabled ? 1 : 0;
    hal_nvs_write(ADDR_LIMIT_ENABLED, &flag, sizeof(flag));

    flag = cal->buzzer_enabled ? 1 : 0;
    hal_nvs_write(ADDR_BUZZER_ENABLED, &flag, sizeof(flag));

    flag = cal->cutoff1_enabled ? 1 : 0;
    hal_nvs_write(ADDR_CUTOFF1_ENABLED, &flag, sizeof(flag));

    flag = cal->cutoff2_enabled ? 1 : 0;
    hal_nvs_write(ADDR_CUTOFF2_ENABLED, &flag, sizeof(flag));

    float mw = cal->max_wind_sensor;
    hal_nvs_write(ADDR_MAX_WIND_SENSOR, &mw, sizeof(mw));

    int32_t adc_max = cal->adc_max;
    hal_nvs_write(ADDR_ADC_MAX, &adc_max, sizeof(adc_max));

    float shunt = cal->shunt_ohms;
    hal_nvs_write(ADDR_SHUNT_OHMS, &shunt, sizeof(shunt));

    int32_t zero = cal->zero_counts;
    hal_nvs_write(ADDR_ZERO_COUNTS, &zero, sizeof(zero));

    int32_t fs = cal->fs_counts;
    hal_nvs_write(ADDR_FS_COUNTS, &fs, sizeof(fs));

    float bl = cal->buzzer_limit;
    hal_nvs_write(ADDR_BUZZER_LIMIT, &bl, sizeof(bl));

    uint16_t bp = cal->buzzer_period_ms;
    hal_nvs_write(ADDR_BUZZER_PERIOD_MS, &bp, sizeof(bp));

    uint8_t magic = CALIBRATION_MAGIC_16BIT;
    hal_nvs_write(ADDR_VERSION_MAGIC, &magic, sizeof(magic));

    return hal_nvs_commit();
}
