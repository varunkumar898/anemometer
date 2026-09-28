#include "mcp3426.h"
#include "hal.h"

mcp3426_dev_t g_mcp3426 = {
    .detected    = false,
    .error_count = 0,
    .last_raw    = -1
};

bool mcp3426_init(void) {
    uint8_t cfg = MCP3426_CONFIG;
    bool ok = hal_i2c_write(MCP3426_I2C_ADDR, &cfg, 1);
    g_mcp3426.detected = ok;
    if (ok) {
        g_mcp3426.error_count = 0;
        hal_uart_printf(">>> MCP3426 OK at 0x%02X (16-bit Continuous)\r\n", MCP3426_I2C_ADDR);
    } else {
        hal_uart_printf(">>> MCP3426 not responding at 0x%02X\r\n", MCP3426_I2C_ADDR);
    }
    return ok;
}

bool mcp3426_is_connected(void) {
    return g_mcp3426.detected;
}

int32_t mcp3426_read_raw(void) {
    if (!g_mcp3426.detected) {
        if (!mcp3426_init()) {
            return -1;
        }
    }

    uint8_t rx[3] = {0, 0, 0};
    bool ok = hal_i2c_read(MCP3426_I2C_ADDR, rx, 3);

    if (ok) {
        g_mcp3426.error_count = 0;
        uint8_t msb = rx[0];
        uint8_t lsb = rx[1];
        /* rx[2] is configuration / ready bit */

        int16_t signed_val = (int16_t)((msb << 8) | lsb);
        if (signed_val < 0) {
            signed_val = 0; /* Sensor shunt voltage is unipolar */
        }
        g_mcp3426.last_raw = (int32_t)signed_val;
        return g_mcp3426.last_raw;
    }

    /* Read failed: increment error count */
    g_mcp3426.error_count++;

    /* If 5 reads fail in a row (~1 sec of communication loss), recover bus */
    if (g_mcp3426.error_count >= 5) {
        g_mcp3426.error_count = 0;
        g_mcp3426.detected = false;
        hal_i2c_recover_bus();
        mcp3426_init();
    }

    return -1;
}
