#ifndef META_TILES_H
#define META_TILES_H

#define VRAM_OFFSET(x,y)  (((y & 31) << 5) + (x & 31))

#include <gbdk/platform.h>
#include "gbs_types.h"
#include "vm.h"
#include "actor.h"

#define MAX_MAP_DATA_SIZE 1024 //32x32 basically mirrors the tilemap VRAM data

extern uint8_t __at(0xBB00) sram_collision_data[256];
extern uint8_t __at(0xBC00) sram_map_data[MAX_MAP_DATA_SIZE]; //0xA000 + (0x2000 (8k SRAM max size) - 0x0400 (MAX_MAP_DATA_SIZE))

extern UBYTE metatile_bank;
extern unsigned char* metatile_ptr;

extern UBYTE metatile_attr_bank;
extern unsigned char* metatile_attr_ptr;

extern UBYTE metatile_collision_bank;
extern unsigned char* metatile_collision_ptr;


void replace_meta_tile(UBYTE x, UBYTE y, UBYTE tile_id) BANKED;

SCRIPT_CTX * create_script_context(void) BANKED;

#endif
