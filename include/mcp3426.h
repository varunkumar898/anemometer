#ifndef MCP3426_H
#define MCP3426_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCP3426_I2C_ADDR   0x68
#define MCP3426_CONFIG     0x98 /* 16-bit, continuous, 15 SPS, 1x gain, CH1 */

typedef struct {
    bool     detected;
    uint8_t  error_count;
    int32_t  last_raw;
} mcp3426_dev_t;

extern mcp3426_dev_t g_mcp3426;

bool    mcp3426_init(void);
int32_t mcp3426_read_raw(void);
bool    mcp3426_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* MCP3426_H */
