#ifndef MMX4_COOP_DEBUG_PROTOCOL_H
#define MMX4_COOP_DEBUG_PROTOCOL_H
#include <stdint.h>

/* Development-only commands ride on the admitted P1 digital pad, rather than
 * asynchronous RAM writes. L3+R3 mark a seven-word, indexed packet. Hold each
 * word until its acknowledgment; normal input or an invalid index aborts it. */
typedef struct {
    uint16_t words[7], previous;
    unsigned next, last_sequence, have_previous;
} Mmx4DebugDecoder;
typedef struct {
    unsigned sequence, command, first, second, third;
    int16_t x, y;
} Mmx4DebugRequest;

int mmx4_debug_is_packet(uint16_t buttons);
int mmx4_debug_decode(Mmx4DebugDecoder *state,uint16_t buttons,Mmx4DebugRequest *out);
uint16_t mmx4_debug_checksum(const uint16_t words[6]);
#endif
