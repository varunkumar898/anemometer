#include "cli.h"
#include "anemometer.h"
#include "calibration.h"
#include "max7219.h"
#include "mcp3426.h"
#include "hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static char     s_cmd_buf[CLI_LINE_MAX_LEN];
static uint16_t s_cmd_pos = 0;
static bool     s_stream_telemetry = false;
static uint32_t s_last_stream_ms   = 0;

void cli_print_prompt(void) {
    hal_uart_puts("anemo> ");
}

static void cmd_help(void) {
    hal_uart_puts("\r\n=== Bare-Metal Industrial Anemometer Commands ===\r\n");
    hal_uart_puts("  help                : Print this command menu\r\n");
    hal_uart_puts("  data                : Return live JSON data {\"wind\":..., \"raw\":...}\r\n");
    hal_uart_puts("  status              : Detailed telemetry and system status\r\n");
    hal_uart_puts("  setlimit <val>      : Set wind alarm limit in m/s (e.g. 20.0)\r\n");
    hal_uart_puts("  setmax <val>        : Set max sensor rating in m/s (e.g. 45.0)\r\n");
    hal_uart_puts("  setbright <0-15>    : Set MAX7219 display intensity\r\n");
    hal_uart_puts("  setzero <counts>    : Set 4mA Zero ADC counts (e.g. 6400)\r\n");
    hal_uart_puts("  setfs <counts>      : Set 20mA Full-Scale ADC counts (e.g. 32000)\r\n");
    hal_uart_puts("  setshunt <ohms>     : Set shunt resistor resistance in ohms (e.g. 100.0)\r\n");
    hal_uart_puts("  setadcmax <counts>  : Set max ADC resolution counts (e.g. 32767)\r\n");
    hal_uart_puts("  alarm <1|0>         : Enable/disable master alarm interlock\r\n");
    hal_uart_puts("  buzzer <1|0>        : Enable/disable audio buzzer output\r\n");
    hal_uart_puts("  setbuzzlimit <val>  : Set buzzer wind limit in m/s (e.g. 15.0)\r\n");
    hal_uart_puts("  setbuzzrate <ms>    : Set buzzer pulse period in ms (e.g. 250, 500, 1000, 0=solid)\r\n");
    hal_uart_puts("  cutoff1 <1|0>       : Enable/disable Cutoff Relay 1\r\n");
    hal_uart_puts("  cutoff2 <1|0>       : Enable/disable Cutoff Relay 2\r\n");
    hal_uart_puts("  save                : Commit configuration to non-volatile storage\r\n");
    hal_uart_puts("  reset               : Restore factory default calibration\r\n");
    hal_uart_puts("  stream <1|0>        : Toggle live telemetry stream\r\n");
    hal_uart_puts("  sim <counts>        : Inject simulated ADC count (-1 to disable)\r\n");
    hal_uart_puts("===================================================\r\n");
}

static void cmd_data(void) {
    const anemometer_state_t *st = anemometer_get_state();
    hal_uart_printf("{\"wind\":\"%.2f\",\"raw\":\"%ld\"}\r\n",
                    (double)st->current_wind_ms,
                    (long)st->current_raw_adc);
}

static void cmd_status(void) {
    const anemometer_state_t *st = anemometer_get_state();
    hal_uart_puts("\r\n--- Anemometer System Status ---\r\n");
    hal_uart_printf("  Current Wind Speed : %.2f m/s\r\n", (double)st->current_wind_ms);
    hal_uart_printf("  Raw ADC Counts     : %ld\r\n", (long)st->current_raw_adc);
    hal_uart_printf("  Filtered ADC       : %.1f\r\n", (double)st->filtered_adc);
    hal_uart_printf("  Sensor Status      : %s\r\n", st->is_fault ? "FAULT / DISCONNECTED" : "HEALTHY");
    hal_uart_printf("  Warmup Grace Period: %s\r\n", st->is_warming_up ? "ACTIVE (Hold)" : "PASSED");
    hal_uart_printf("  Wind Limit Alarm   : %s (Limit: %.2f m/s)\r\n",
                    st->exceed_limit ? "EXCEEDED" : "NORMAL", (double)g_cal.wind_limit);
    hal_uart_printf("  Alarm Interlock    : %s\r\n", st->alarm_triggered ? "TRIGGERED" : "OFF");
    hal_uart_printf("  Buzzer Output      : %s (Limit: %.2f m/s, Rate: %u ms, Enabled: %s)\r\n",
                    st->buzzer_active ? "ACTIVE" : "OFF",
                    (double)g_cal.buzzer_limit,
                    (unsigned int)g_cal.buzzer_period_ms,
                    g_cal.buzzer_enabled ? "YES" : "NO");
    hal_uart_printf("  Cutoff Relay 1     : %s (Config: %s)\r\n",
                    st->cutoff1_active ? "TRIGGERED" : "OFF", g_cal.cutoff1_enabled ? "ON" : "OFF");
    hal_uart_printf("  Cutoff Relay 2     : %s (Config: %s)\r\n",
                    st->cutoff2_active ? "TRIGGERED" : "OFF", g_cal.cutoff2_enabled ? "ON" : "OFF");
    hal_uart_printf("  LED Indicators     : RED=%s, GREEN=%s\r\n",
                    st->red_led_active ? "ON" : "OFF", st->green_led_active ? "ON" : "OFF");
    hal_uart_printf("  Display Brightness : %d / 15\r\n", (int)g_cal.brightness);
    hal_uart_printf("  Zero / FS Counts   : %ld / %ld (Shunt: %.1f ohm)\r\n",
                    (long)g_cal.zero_counts, (long)g_cal.fs_counts, (double)g_cal.shunt_ohms);
    hal_uart_puts("--------------------------------\r\n");
}

static void process_command(char *line) {
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') {
        cli_print_prompt();
        return;
    }

    char *cmd = strtok(line, " \t\r\n");
    char *arg = strtok(NULL, " \t\r\n");

    if (!cmd) {
        cli_print_prompt();
        return;
    }

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "data") == 0) {
        cmd_data();
    } else if (strcmp(cmd, "status") == 0) {
        cmd_status();
    } else if (strcmp(cmd, "setlimit") == 0 && arg) {
        float val = (float)atof(arg);
        if (val >= 0.0f && val <= 200.0f) {
            g_cal.wind_limit = val;
            hal_uart_printf("OK: Wind limit set to %.2f m/s\r\n", (double)val);
        } else {
            hal_uart_puts("ERR: Limit must be between 0 and 200 m/s\r\n");
        }
    } else if (strcmp(cmd, "setmax") == 0 && arg) {
        float val = (float)atof(arg);
        if (val > 0.0f && val <= 200.0f) {
            g_cal.max_wind_sensor = val;
            hal_uart_printf("OK: Max wind rating set to %.2f m/s\r\n", (double)val);
        } else {
            hal_uart_puts("ERR: Max wind rating must be between 1 and 200 m/s\r\n");
        }
    } else if (strcmp(cmd, "setbright") == 0 && arg) {
        int b = atoi(arg);
        if (b >= 0 && b <= 15) {
            g_cal.brightness = (uint8_t)b;
            max7219_set_brightness((uint8_t)b);
            hal_uart_printf("OK: Brightness set to %d\r\n", b);
        } else {
            hal_uart_puts("ERR: Brightness must be 0 to 15\r\n");
        }
    } else if (strcmp(cmd, "setzero") == 0 && arg) {
        long z = atol(arg);
        if (z >= 0 && z <= 65535) {
            g_cal.zero_counts = (int32_t)z;
            hal_uart_printf("OK: Zero counts set to %ld\r\n", z);
        } else {
            hal_uart_puts("ERR: Zero counts must be between 0 and 65535\r\n");
        }
    } else if (strcmp(cmd, "setfs") == 0 && arg) {
        long fs = atol(arg);
        if (fs >= 0 && fs <= 65535) {
            g_cal.fs_counts = (int32_t)fs;
            hal_uart_printf("OK: FS counts set to %ld\r\n", fs);
        } else {
            hal_uart_puts("ERR: FS counts must be between 0 and 65535\r\n");
        }
    } else if (strcmp(cmd, "setshunt") == 0 && arg) {
        float sh = (float)atof(arg);
        if (sh >= 0.1f && sh <= 10000.0f) {
            g_cal.shunt_ohms = sh;
            hal_uart_printf("OK: Shunt resistance set to %.1f ohms\r\n", (double)sh);
        } else {
            hal_uart_puts("ERR: Shunt resistance must be between 0.1 and 10000 ohms\r\n");
        }
    } else if (strcmp(cmd, "setadcmax") == 0 && arg) {
        long m = atol(arg);
        if (m >= 1 && m <= 65535) {
            g_cal.adc_max = (int32_t)m;
            hal_uart_printf("OK: ADC max counts set to %ld\r\n", m);
        } else {
            hal_uart_puts("ERR: ADC max must be between 1 and 65535\r\n");
        }
    } else if (strcmp(cmd, "alarm") == 0 && arg) {
        g_cal.limit_enabled = (atoi(arg) != 0);
        hal_uart_printf("OK: Master alarm interlock is now %s\r\n", g_cal.limit_enabled ? "ENABLED" : "DISABLED");
    } else if (strcmp(cmd, "buzzer") == 0 && arg) {
        g_cal.buzzer_enabled = (atoi(arg) != 0);
        hal_uart_printf("OK: Buzzer is now %s\r\n", g_cal.buzzer_enabled ? "ENABLED" : "DISABLED");
    } else if (strcmp(cmd, "setbuzzlimit") == 0 && arg) {
        float val = (float)atof(arg);
        if (val >= 0.0f && val <= 200.0f) {
            g_cal.buzzer_limit = val;
            hal_uart_printf("OK: Buzzer wind limit set to %.2f m/s\r\n", (double)val);
        } else {
            hal_uart_puts("ERR: Buzzer limit must be between 0 and 200 m/s\r\n");
        }
    } else if (strcmp(cmd, "setbuzzrate") == 0 && arg) {
        int rate = atoi(arg);
        if (rate >= 0 && rate <= 5000) {
            g_cal.buzzer_period_ms = (uint16_t)rate;
            if (rate == 0) {
                hal_uart_puts("OK: Buzzer rate set to continuous solid tone\r\n");
            } else {
                hal_uart_printf("OK: Buzzer pulse period set to %d ms\r\n", rate);
            }
        } else {
            hal_uart_puts("ERR: Buzzer rate must be between 0 and 5000 ms\r\n");
        }
    } else if (strcmp(cmd, "cutoff1") == 0 && arg) {
        g_cal.cutoff1_enabled = (atoi(arg) != 0);
        hal_uart_printf("OK: Cutoff 1 is now %s\r\n", g_cal.cutoff1_enabled ? "ENABLED" : "DISABLED");
    } else if (strcmp(cmd, "cutoff2") == 0 && arg) {
        g_cal.cutoff2_enabled = (atoi(arg) != 0);
        hal_uart_printf("OK: Cutoff 2 is now %s\r\n", g_cal.cutoff2_enabled ? "ENABLED" : "DISABLED");
    } else if (strcmp(cmd, "save") == 0) {
        bool ok = calibration_save(&g_cal);
        hal_uart_printf("%s: Calibration %s\r\n", ok ? "OK" : "ERR", ok ? "committed to NV storage" : "commit failed");
    } else if (strcmp(cmd, "reset") == 0) {
        calibration_set_defaults(&g_cal);
        calibration_save(&g_cal);
        max7219_set_brightness(g_cal.brightness);
        hal_uart_puts("OK: Factory defaults restored and saved\r\n");
    } else if (strcmp(cmd, "stream") == 0 && arg) {
        s_stream_telemetry = (atoi(arg) != 0);
        hal_uart_printf("OK: Telemetry stream %s\r\n", s_stream_telemetry ? "ON" : "OFF");
    } else if (strcmp(cmd, "sim") == 0 && arg) {
        long count = atol(arg);
        anemometer_inject_raw((int32_t)count);
        if (count >= 0) {
            hal_uart_printf("OK: Injected simulated ADC count: %ld\r\n", count);
        } else {
            hal_uart_puts("OK: Simulated ADC disabled, reverting to hardware ADC\r\n");
        }
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
        hal_uart_puts("Exiting simulation...\r\n");
        exit(0);
    } else {
        hal_uart_printf("Unknown command: '%s'. Type 'help' for available commands.\r\n", cmd);
    }

    cli_print_prompt();
}

void cli_init(void) {
    s_cmd_pos = 0;
    s_stream_telemetry = false;
    hal_uart_puts("\r\n");
    hal_uart_puts("════════════════════════════════════════════════\r\n");
    hal_uart_puts("  Bare-Metal Industrial Anemometer Firmware    \r\n");
    hal_uart_puts("  16-Bit MCP3426 ADC + MAX7219 7-Segment Display \r\n");
    hal_uart_puts("════════════════════════════════════════════════\r\n");
    hal_uart_puts("Type 'help' for command list. Type 'data' for JSON.\r\n\r\n");
    cli_print_prompt();
}

void cli_poll(void) {
    /* Handle optional continuous telemetry streaming */
    if (s_stream_telemetry) {
        uint32_t now = hal_get_millis();
        if (now - s_last_stream_ms >= 500) {
            s_last_stream_ms = now;
            cmd_data();
        }
    }

    while (hal_uart_readable()) {
        char c = hal_uart_getc();

        if (c == '\r' || c == '\n') {
            hal_uart_puts("\r\n");
            s_cmd_buf[s_cmd_pos] = '\0';
            process_command(s_cmd_buf);
            s_cmd_pos = 0;
        } else if (c == '\b' || c == 0x7F) { /* Backspace */
            if (s_cmd_pos > 0) {
                s_cmd_pos--;
                hal_uart_puts("\b \b");
            }
        } else if (s_cmd_pos < sizeof(s_cmd_buf) - 1) {
            s_cmd_buf[s_cmd_pos++] = c;
            hal_uart_putc(c); /* Echo */
        }
    }
}
