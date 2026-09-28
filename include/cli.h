#ifndef CLI_H
#define CLI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_LINE_MAX_LEN 96

void cli_init(void);
void cli_poll(void);
void cli_print_prompt(void);

#ifdef __cplusplus
}
#endif

#endif /* CLI_H */
