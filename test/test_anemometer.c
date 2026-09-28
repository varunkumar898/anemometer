#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "anemometer.h"
#include "calibration.h"
#include "hal.h"

int main(void) {
    printf("Running Anemometer Unit Tests...\n");

    hal_init();
    calibration_set_defaults(&g_cal);

    /* Test 1: Baseline Zero Counts */
    assert(g_cal.zero_counts == 6400);
    assert(g_cal.fs_counts == 32000);
    assert(g_cal.max_wind_sensor == 45.00f);

    anemometer_init();

    /* Force warmup to be finished by manipulating boot_time */
    g_anemo.boot_time_ms = 0;

    /* Test 2: Transfer function at zero count (4mA = 6400 counts) */
    anemometer_inject_raw(6400);
    /* Run tick at 250ms */
    anemometer_tick(250);
    printf("ADC 6400 -> Wind: %.2f m/s (Expected: 0.00)\n", (double)g_anemo.current_wind_ms);
    assert(fabs(g_anemo.current_wind_ms - 0.0f) < 0.05f);

    /* Test 3: Transfer function at mid-scale (20000 counts) */
    /* Feed 50 ticks to allow EMA filter (alpha=0.2) to fully settle */
    uint32_t t = 500;
    for (int i = 0; i < 50; i++) {
        t += 200;
        anemometer_inject_raw(20000);
        anemometer_tick(t);
    }
    /* Theoretical: (20000 - 6400) * (45.0 / (32000 - 6400)) = 13600 * 45 / 25600 = 23.906 m/s */
    printf("ADC 20000 -> Converged Wind: %.2f m/s (Expected: ~23.91 m/s)\n", (double)g_anemo.current_wind_ms);
    assert(fabs(g_anemo.current_wind_ms - 23.91f) < 0.15f);

    /* Test 4: Alarm condition */
    /* Wind limit is 20.0 m/s. 23.91 > 20.0, so exceed_limit must be true */
    assert(g_anemo.exceed_limit == true);
    assert(g_anemo.red_led_active == true);
    assert(g_anemo.green_led_active == false);

    /* Test 5: Full-Scale Counts (32000 counts = 45 m/s) */
    for (int i = 0; i < 50; i++) {
        t += 200;
        anemometer_inject_raw(32000);
        anemometer_tick(t);
    }
    printf("ADC 32000 -> Converged Wind: %.2f m/s (Expected: ~45.00 m/s)\n", (double)g_anemo.current_wind_ms);
    assert(fabs(g_anemo.current_wind_ms - 45.00f) < 0.15f);

    /* Test 6: Deadband (< 0.2 m/s clamps to 0.0 m/s) */
    /* 6450 counts -> ~0.088 m/s -> clamped to 0.0 m/s */
    for (int i = 0; i < 50; i++) {
        t += 200;
        anemometer_inject_raw(6450);
        anemometer_tick(t);
    }
    printf("ADC 6450 -> Wind: %.2f m/s (Expected: 0.00 m/s due to deadband)\n", (double)g_anemo.current_wind_ms);
    assert(g_anemo.current_wind_ms == 0.0f);

    /* Test 7: Fault detection (disconnected wire / open circuit: adc < 100) */
    for (int i = 0; i < 50; i++) {
        t += 200;
        anemometer_inject_raw(20);
        anemometer_tick(t);
    }
    printf("ADC 20 -> Fault status: %s (Expected: FAULT)\n", g_anemo.is_fault ? "FAULT" : "OK");
    assert(g_anemo.is_fault == true);
    assert(g_anemo.current_wind_ms == 0.0f);

    printf("ALL ANEMOMETER TESTS PASSED!\n");
    return 0;
}
