#include "anemometer.h"
#include "mcp3426.h"
#include "max7219.h"
#include "calibration.h"
#include "hal.h"

anemometer_state_t g_anemo;

static int32_t s_injected_raw = -1;

void anemometer_inject_raw(int32_t raw_adc) {
    s_injected_raw = raw_adc;
}

void anemometer_init(void) {
    g_anemo.current_wind_ms   = 0.0f;
    g_anemo.current_raw_adc   = 0;
    g_anemo.filtered_adc      = -1.0f;
    g_anemo.is_fault          = false;
    g_anemo.is_warming_up     = true;
    g_anemo.exceed_limit      = false;
    g_anemo.alarm_triggered   = false;
    g_anemo.buzzer_active     = false;
    g_anemo.cutoff1_active    = false;
    g_anemo.cutoff2_active    = false;
    g_anemo.red_led_active    = false;
    g_anemo.green_led_active  = true;
    g_anemo.boot_time_ms      = hal_get_millis();
    g_anemo.last_sample_ms    = 0;
    g_anemo.last_blink_ms     = 0;
    g_anemo.blink_state       = true;

    /* Pre-load safe initial pin states before operation */
    hal_gpio_write(HAL_PIN_BUZZER,    HAL_PIN_LOW);
    hal_gpio_write(HAL_PIN_CUTOFF1,   HAL_PIN_LOW);
    hal_gpio_write(HAL_PIN_CUTOFF2,   HAL_PIN_LOW);
    hal_gpio_write(HAL_PIN_LED_RED,   HAL_PIN_LOW);
    hal_gpio_write(HAL_PIN_LED_GREEN, HAL_PIN_HIGH);
}

const anemometer_state_t *anemometer_get_state(void) {
    return &g_anemo;
}

void anemometer_tick(uint32_t now_ms) {
    if (now_ms - g_anemo.last_sample_ms < ANEMOMETER_SAMPLE_INTERVAL_MS) {
        return;
    }
    g_anemo.last_sample_ms = now_ms;

    /* Obtain ADC raw count from MCP3426 or injection */
    int32_t raw_val;
    if (s_injected_raw >= 0) {
        raw_val = s_injected_raw;
    } else {
        raw_val = mcp3426_read_raw();
    }

    /* EMA filter (alpha = 0.2) */
    if (raw_val >= 0) {
        if (g_anemo.filtered_adc < 0.0f) {
            g_anemo.filtered_adc = (float)raw_val;
        } else {
            g_anemo.filtered_adc = (g_anemo.filtered_adc * 0.8f) + ((float)raw_val * 0.2f);
        }
    }

    int32_t adc = (g_anemo.filtered_adc >= 0.0f) ? (int32_t)g_anemo.filtered_adc : 0;

    /* 4-20 mA transfer function -> wind speed in m/s */
    float span = (float)(g_cal.fs_counts - g_cal.zero_counts);
    float wind_ms = 0.0f;
    if (span > 0.0f) {
        wind_ms = (float)(adc - g_cal.zero_counts) * (g_cal.max_wind_sensor / span);
    }

    /* Open-circuit / Disconnected / Saturation Fault Detection */
    bool is_fault = false;
    if (g_cal.zero_counts > 500 && adc < 100) {
        is_fault = true;
    }
    if (adc > g_cal.adc_max + 1000) {
        is_fault = true;
    }

    if (is_fault) {
        wind_ms = 0.0f;
    } else {
        if (wind_ms < 0.0f)                 wind_ms = 0.0f;
        if (wind_ms > g_cal.max_wind_sensor) wind_ms = g_cal.max_wind_sensor;
        if (wind_ms < 0.2f)                 wind_ms = 0.0f; /* Deadband */
    }

    g_anemo.current_raw_adc = adc;
    g_anemo.current_wind_ms = wind_ms;
    g_anemo.is_fault        = is_fault;

    /* 4-second power-up stabilization grace period: hold alarms OFF */
    bool is_warming_up = (now_ms - g_anemo.boot_time_ms < ANEMOMETER_WARMUP_TIME_MS);
    g_anemo.is_warming_up = is_warming_up;

    bool exceed_limit = (!is_warming_up) && (g_anemo.current_wind_ms >= g_cal.wind_limit);
    g_anemo.exceed_limit = exceed_limit;

    bool alarm_triggered = g_cal.limit_enabled && exceed_limit;
    g_anemo.alarm_triggered = alarm_triggered;

    /* Status LEDs */
    g_anemo.red_led_active   = exceed_limit;
    g_anemo.green_led_active = !exceed_limit;
    hal_gpio_write(HAL_PIN_LED_RED,   exceed_limit ? HAL_PIN_HIGH : HAL_PIN_LOW);
    hal_gpio_write(HAL_PIN_LED_GREEN, exceed_limit ? HAL_PIN_LOW  : HAL_PIN_HIGH);

    /* 7-Segment Display update */
    if (alarm_triggered) {
        if (now_ms - g_anemo.last_blink_ms > ANEMOMETER_ALARM_BLINK_MS) {
            g_anemo.last_blink_ms = now_ms;
            g_anemo.blink_state = !g_anemo.blink_state;
        }
        if (g_anemo.blink_state) {
            max7219_display_wid();
        } else {
            max7219_clear();
        }
    } else {
        max7219_update_wind(g_anemo.current_wind_ms);
    }

    /* Configurable Audible Buzzer: independent wind limit threshold & pulse rate */
    float b_limit = (g_cal.buzzer_limit > 0.0f) ? g_cal.buzzer_limit : g_cal.wind_limit;
    bool exceed_buzz = (!is_warming_up) && (g_anemo.current_wind_ms >= b_limit);
    bool buzz_active = g_cal.buzzer_enabled && (exceed_buzz || alarm_triggered);

    bool buzz_on = false;
    if (buzz_active) {
        uint16_t period = g_cal.buzzer_period_ms;
        if (period == 0) {
            buzz_on = true; /* Continuous solid tone */
        } else {
            buzz_on = (((now_ms / period) % 2) == 0); /* Pulsed tone */
        }
    }
    g_anemo.buzzer_active = buzz_on;
    hal_gpio_write(HAL_PIN_BUZZER, buzz_on ? HAL_PIN_HIGH : HAL_PIN_LOW);

    /* Cutoff Interlock Relays */
    bool c1 = g_cal.cutoff1_enabled && alarm_triggered;
    bool c2 = g_cal.cutoff2_enabled && alarm_triggered;
    g_anemo.cutoff1_active = c1;
    g_anemo.cutoff2_active = c2;
    hal_gpio_write(HAL_PIN_CUTOFF1, c1 ? HAL_PIN_HIGH : HAL_PIN_LOW);
    hal_gpio_write(HAL_PIN_CUTOFF2, c2 ? HAL_PIN_HIGH : HAL_PIN_LOW);
}
