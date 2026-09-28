#include "hal.h"
#include "calibration.h"
#include "mcp3426.h"
#include "max7219.h"
#include "anemometer.h"
#include "cli.h"

int main(void) {
    /* 1. Low-level Hardware Initialization */
    hal_init();

    /* 2. Serial UART Console (115200 8N1) */
    hal_uart_init(115200);

    /* 3. Non-Volatile Calibration */
    calibration_load(&g_cal);

    /* 4. MAX7219 7-Segment Display */
    max7219_init(g_cal.brightness);

    /* 5. MCP3426 16-Bit I2C ADC */
    mcp3426_init();

    /* 6. Anemometer Engine & Safe Default Interlocks */
    anemometer_init();

    /* 7. Interactive Serial CLI */
    cli_init();

    /* 8. Bare-Metal Superloop */
    while (1) {
        uint32_t now = hal_get_millis();

        /* Process ADC reading, EMA filter, alarms, and display updates */
        anemometer_tick(now);

        /* Poll UART for user commands and live telemetry */
        cli_poll();
    }

    return 0;
}
