#ifndef MMX4_COOP_TEXTURE_CACHE_H
#define MMX4_COOP_TEXTURE_CACHE_H
#include <stdint.h>

/* Find a reusable ID without replacing artwork referenced by either native
 * packet buffer. The selection depends only on saved guest memory, not host
 * clock/LRU history, so restored timelines make the same decision. */
unsigned mmx4_coop_texture_cache_slot(unsigned count, uint16_t first,
    unsigned start, uint32_t world, uint32_t hud,
    uint32_t (*read_word)(uint32_t));
/* Each native actor owns one arena in each DMA frame. Four-player mode
 * reserves eight arenas; every one must pin its still-referenced bank IDs. */
unsigned mmx4_coop_texture_cache_slot_buffers(unsigned count,uint16_t first,
    unsigned start,uint32_t world,uint32_t hud,unsigned world_buffers,
    uint32_t (*read_word)(uint32_t));
#endif
