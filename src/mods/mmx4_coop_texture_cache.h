#ifndef MMX4_COOP_TEXTURE_CACHE_H
#define MMX4_COOP_TEXTURE_CACHE_H
#include <stdint.h>

/* 1,000 native 40-byte packets plus their two 40-byte expanded triangles.
 * Eight frame slots and the HUD must coexist with all six 1 MiB background
 * planes inside the GPU's fixed 8 MiB DMA aperture. */
#define MMX4_COOP_FRAME_ARENA_BYTES 0x20000u
#define MMX4_COOP_HUD_ARENA_BYTES 0x10000u

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
