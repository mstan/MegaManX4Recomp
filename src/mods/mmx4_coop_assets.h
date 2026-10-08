#ifndef MMX4_COOP_ASSETS_H
#define MMX4_COOP_ASSETS_H
#include <stddef.h>
#include <stdint.h>
typedef struct { const uint8_t *data; size_t size; uint32_t type; } Mmx4Asset;
/* Original SLUS-00561 ARC descriptor/sector format, loader 80013E68. */
int mmx4_coop_arc_asset(const uint8_t *data, size_t size, unsigned member, Mmx4Asset *out);
/* Original word LZ format at 80016FF4, also used by X6 at 80018BE0. */
int mmx4_coop_decode_sprite(const uint8_t *src, size_t size, uint8_t *dst,
                            size_t capacity, size_t *written);
#endif
