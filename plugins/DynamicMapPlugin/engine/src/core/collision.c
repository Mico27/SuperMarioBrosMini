#pragma bank 255

#include <gbdk/platform.h>

#include "collision.h"
#include "math.h"
#include "bankdata.h"
#include "meta_tiles.h"
#include "data_manager.h"

/**
 * Check if point is within positioned bounding box.
 *
 * @param bb Pointer to bounding box
 * @param offset Pointer to position offset for bounding box (e.g Actor position)
 * @param point Pointer to position to look for within bounding box
 * @return Point is within bounding box
 */
UBYTE bb_contains(bounding_box_t *bb, point16_t *offset, point16_t *point) BANKED {
    if ((point->x < (offset->x >> 4) + bb->left) || 
        (point->x > (offset->x >> 4) + bb->right)) return FALSE;
    if ((point->y < (offset->y >> 4) + bb->top) || 
        (point->y > (offset->y >> 4) + bb->bottom)) return FALSE;
    return TRUE;
}

/**
 * Check if two positioned bounding boxes intersect.
 *
 * @param bb_a Pointer to bounding box A
 * @param offset_a Pointer to position offset for bounding box A
 * @param bb_b Pointer to bounding box B
 * @param offset_b Pointer to position offset for bounding box B
 * @return Positioned bounding boxes intersect
 */
UBYTE bb_intersects(bounding_box_t *bb_a, point16_t *offset_a, bounding_box_t *bb_b, point16_t *offset_b) BANKED {
    if (((offset_b->x >> 4) + bb_b->left   > (offset_a->x >> 4) + bb_a->right) ||
        ((offset_b->x >> 4) + bb_b->right  < (offset_a->x >> 4) + bb_a->left)) return FALSE;
    if (((offset_b->y >> 4) + bb_b->top    > (offset_a->y >> 4) + bb_a->bottom) ||
        ((offset_b->y >> 4) + bb_b->bottom < (offset_a->y >> 4) + bb_a->top)) return FALSE;
    return TRUE;
}

/**
 * Return collision tile value at given tile x,y coordinate.
 *
 * @param tx Left tile
 * @param ty Top tile
 * @return Tile value, 0 if no collisions, COLLISION_ALL if out of bounds
 */
UBYTE tile_at(UBYTE tx, UBYTE ty) BANKED {
	if (tx < image_tile_width && ty < image_tile_height) {
		if (metatile_collision_bank) {
			//return ReadBankedUBYTE(metatile_collision_ptr + sram_map_data[VRAM_OFFSET(tx, ty)], metatile_collision_bank);
			return sram_collision_data[sram_map_data[VRAM_OFFSET(tx, ty)]];
		} else {
			return ReadBankedUBYTE(collision_ptr + (ty * (UINT16)image_tile_width) + tx, collision_bank);
		}
	}
    return 0;//COLLISION_ALL;
}
