#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A replay overrides accepted seat input, never physical device bindings. */
int mmx4_diagnostics_resolve_input(unsigned seat,uint16_t *buttons);
void mmx4_diagnostics_reset(void);
#ifdef __cplusplus
}
#endif
