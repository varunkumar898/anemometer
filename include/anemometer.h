#ifndef ANEMOMETER_H
#define ANEMOMETER_H

#include <stdint.h>
#include <stdbool.h>
#include "calibration.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ANEMOMETER_SAMPLE_INTERVAL_MS 200
#define ANEMOMETER_WARMUP_TIME_MS     4000
#define ANEMOMETER_BUZZER_PERIOD_MS   500
#define ANEMOMETER_ALARM_BLINK_MS     500

typedef struct {
    float    current_wind_ms;
    int32_t  current_raw_adc;
    float    filtered_adc;
    bool     is_fault;
    bool     is_warming_up;
    bool     exceed_limit;
    bool     alarm_triggered;
    bool     buzzer_active;
    bool     cutoff1_active;
    bool     cutoff2_active;
    bool     red_led_active;
    bool     green_led_active;
    uint32_t boot_time_ms;
    uint32_t last_sample_ms;
    uint32_t last_blink_ms;
    bool     blink_state;
} anemometer_state_t;

extern anemometer_state_t g_anemo;

void                      anemometer_init(void);
void                      anemometer_tick(uint32_t now_ms);
const anemometer_state_t *anemometer_get_state(void);
void                      anemometer_inject_raw(int32_t raw_adc); /* Useful for testing / simulation */

#ifdef __cplusplus
}
#endif

#endif /* ANEMOMETER_H */
