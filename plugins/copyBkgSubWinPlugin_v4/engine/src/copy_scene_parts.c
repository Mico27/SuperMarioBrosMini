#pragma bank 255

#include <gbdk/platform.h>
#include "system.h"
#include "vm.h"
#include "gbs_types.h"
#include "scroll.h"
#include "bankdata.h"
#include "data_manager.h"

UBYTE tmp_tile_buffer[32];

void set_xy_win_submap(const UBYTE * source, UBYTE bank, UBYTE width, UBYTE x, UBYTE y, UBYTE w, UBYTE h) OLDCALL;

void copy_background_submap_to_overlay(SCRIPT_CTX * THIS) OLDCALL BANKED {
	
	uint8_t source_x = *(int8_t*)VM_REF_TO_PTR(FN_ARG0);
	uint8_t source_y = *(int8_t*)VM_REF_TO_PTR(FN_ARG1);
	uint8_t dest_x = *(int8_t*)VM_REF_TO_PTR(FN_ARG2);
	uint8_t dest_y = *(int8_t*)VM_REF_TO_PTR(FN_ARG3);
	uint8_t width = *(int8_t*)VM_REF_TO_PTR(FN_ARG4);
	uint8_t height = *(int8_t*)VM_REF_TO_PTR(FN_ARG5);
	uint8_t scene_bank = *(uint8_t *) VM_REF_TO_PTR(FN_ARG6);
	const scene_t * scene_ptr = *(scene_t **) VM_REF_TO_PTR(FN_ARG7);		
	scene_t scn;
    MemcpyBanked(&scn, scene_ptr, sizeof(scn), scene_bank);
	background_t bkg;
    MemcpyBanked(&bkg, scn.background.ptr, sizeof(bkg), scn.background.bank);
    unsigned char* tilemap_ptr = bkg.tilemap.ptr;
	unsigned char* tilemap_attr_ptr = bkg.cgb_tilemap_attr.ptr;		
	int16_t offset = (source_y * (int16_t)bkg.width) + source_x;
#ifdef CGB
    if (_is_CGB) {
        VBK_REG = 1;
        set_xy_win_submap(tilemap_attr_ptr + offset,  bkg.cgb_tilemap_attr.bank, bkg.width, dest_x, dest_y, width, height);
        VBK_REG = 0;
    }
#endif
    set_xy_win_submap(tilemap_ptr + offset, bkg.tilemap.bank, bkg.width, dest_x, dest_y, width, height);
	
}

void copy_background_submap_to_overlay_base(SCRIPT_CTX * THIS) OLDCALL BANKED {
	int16_t bkg_pos = *(int16_t*)VM_REF_TO_PTR(FN_ARG0);
	int16_t dest_pos = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
	int16_t wh = *(int16_t*)VM_REF_TO_PTR(FN_ARG2);
	uint8_t tile_idx_offset = *(int8_t*)VM_REF_TO_PTR(FN_ARG3);
	uint8_t scene_bank = *(uint8_t *) VM_REF_TO_PTR(FN_ARG4);
	const scene_t * scene_ptr = *(scene_t **) VM_REF_TO_PTR(FN_ARG5);	

	UBYTE source_x = bkg_pos & 0xFF;
	UBYTE source_y = (bkg_pos >> 8) & 0xFF;
	UBYTE dest_x = dest_pos & 0xFF;
	UBYTE dest_y = (dest_pos >> 8) & 0xFF;
	UBYTE width = (wh & 0xFF) & 31;
	UBYTE height = ((wh >> 8) & 0xFF) & 31;
	
	scene_t scn;
    MemcpyBanked(&scn, scene_ptr, sizeof(scn), scene_bank);
	background_t bkg;
    MemcpyBanked(&bkg, scn.background.ptr, sizeof(bkg), scn.background.bank);
    unsigned char* tilemap_ptr = bkg.tilemap.ptr;
	unsigned char* tilemap_attr_ptr = bkg.cgb_tilemap_attr.ptr;		
		
	UBYTE buffer_size = sizeof(UBYTE) * width;
	for (uint8_t i = 0; i < height; i++){		
		int16_t offset = ((source_y + i) * (int16_t)bkg.width) + source_x;
#ifdef CGB
		if (_is_CGB) {
			VBK_REG = 1;
			MemcpyBanked(tmp_tile_buffer, tilemap_attr_ptr + offset, buffer_size, bkg.cgb_tilemap_attr.bank);
			set_win_tiles(dest_x & 31, (dest_y + i) & 31, width, 1, tmp_tile_buffer);
			VBK_REG = 0;
		}
#endif
		MemcpyBanked(tmp_tile_buffer, tilemap_ptr + offset, buffer_size, bkg.tilemap.bank);
		set_win_based_tiles(dest_x & 31, (dest_y + i) & 31, width, 1, tmp_tile_buffer, tile_idx_offset);
	}	
}


void copy_background_submap_to_background(SCRIPT_CTX * THIS) OLDCALL BANKED {
	
	uint8_t source_x = *(int8_t*)VM_REF_TO_PTR(FN_ARG0);
	uint8_t source_y = *(int8_t*)VM_REF_TO_PTR(FN_ARG1);
	uint8_t dest_x = *(int8_t*)VM_REF_TO_PTR(FN_ARG2);
	uint8_t dest_y = *(int8_t*)VM_REF_TO_PTR(FN_ARG3);
	uint8_t width = *(int8_t*)VM_REF_TO_PTR(FN_ARG4) & 31;
	uint8_t height = *(int8_t*)VM_REF_TO_PTR(FN_ARG5) & 31;
	uint8_t scene_bank = *(uint8_t *) VM_REF_TO_PTR(FN_ARG6);
	const scene_t * scene_ptr = *(scene_t **) VM_REF_TO_PTR(FN_ARG7);		
	scene_t scn;
    MemcpyBanked(&scn, scene_ptr, sizeof(scn), scene_bank);
	background_t bkg;
    MemcpyBanked(&bkg, scn.background.ptr, sizeof(bkg), scn.background.bank);
    unsigned char* tilemap_ptr = bkg.tilemap.ptr;
	unsigned char* tilemap_attr_ptr = bkg.cgb_tilemap_attr.ptr;		
	
	UBYTE buffer_size = sizeof(UBYTE) * width;
	for (uint8_t i = 0; i < height; i++){		
		int16_t offset = ((source_y + i) * (int16_t)bkg.width) + source_x;
#ifdef CGB
		if (_is_CGB) {
			VBK_REG = 1;
			MemcpyBanked(tmp_tile_buffer, tilemap_attr_ptr + offset, buffer_size, bkg.cgb_tilemap_attr.bank);
			set_bkg_tiles(dest_x & 31, (dest_y + i) & 31, width, 1, tmp_tile_buffer);
			VBK_REG = 0;
		}
#endif
		MemcpyBanked(tmp_tile_buffer, tilemap_ptr + offset, buffer_size, bkg.tilemap.bank);
		set_bkg_tiles(dest_x & 31, (dest_y + i) & 31, width, 1, tmp_tile_buffer);
	}
	
}

void copy_background_submap_to_background_base(SCRIPT_CTX * THIS) OLDCALL BANKED {
	int16_t bkg_pos = *(int16_t*)VM_REF_TO_PTR(FN_ARG0);
	int16_t dest_pos = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
	int16_t wh = *(int16_t*)VM_REF_TO_PTR(FN_ARG2);
	uint8_t tile_idx_offset = *(int8_t*)VM_REF_TO_PTR(FN_ARG3);
	uint8_t scene_bank = *(uint8_t *) VM_REF_TO_PTR(FN_ARG4);
	const scene_t * scene_ptr = *(scene_t **) VM_REF_TO_PTR(FN_ARG5);	

	UBYTE source_x = bkg_pos & 0xFF;
	UBYTE source_y = (bkg_pos >> 8) & 0xFF;
	UBYTE dest_x = dest_pos & 0xFF;
	UBYTE dest_y = (dest_pos >> 8) & 0xFF;
	UBYTE width = (wh & 0xFF) & 31;
	UBYTE height = ((wh >> 8) & 0xFF) & 31;
	
	scene_t scn;
    MemcpyBanked(&scn, scene_ptr, sizeof(scn), scene_bank);
	background_t bkg;
    MemcpyBanked(&bkg, scn.background.ptr, sizeof(bkg), scn.background.bank);
    unsigned char* tilemap_ptr = bkg.tilemap.ptr;
	unsigned char* tilemap_attr_ptr = bkg.cgb_tilemap_attr.ptr;		
	
	UBYTE buffer_size = sizeof(UBYTE) * width;	
	
	for (uint8_t i = 0; i < height; i++){		
		int16_t offset = ((source_y + i) * (int16_t)bkg.width) + source_x;
#ifdef CGB
		if (_is_CGB) {
			VBK_REG = 1;
			MemcpyBanked(tmp_tile_buffer, tilemap_attr_ptr + offset, buffer_size, bkg.cgb_tilemap_attr.bank);
			set_bkg_tiles(dest_x & 31, (dest_y + i) & 31, width, 1, tmp_tile_buffer);
			VBK_REG = 0;
		}
#endif
		MemcpyBanked(tmp_tile_buffer, tilemap_ptr + offset, buffer_size, bkg.tilemap.bank);
		set_bkg_based_tiles(dest_x & 31, (dest_y + i) & 31, width, 1, tmp_tile_buffer, tile_idx_offset);
	}	
}

void copy_background_submap_to_tileset(SCRIPT_CTX * THIS) OLDCALL BANKED {	
	int16_t source_pos = *(int16_t*)VM_REF_TO_PTR(FN_ARG0);
	int16_t dest_pos = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
	int16_t wh = *(int16_t*)VM_REF_TO_PTR(FN_ARG2);
	int16_t overlay_pos = *(int16_t*)VM_REF_TO_PTR(FN_ARG3);	
	uint8_t copy_attributes = *(int8_t*)VM_REF_TO_PTR(FN_ARG4);	
	uint8_t scene_bank = *(uint8_t *) VM_REF_TO_PTR(FN_ARG5);
	const scene_t * scene_ptr = *(scene_t **) VM_REF_TO_PTR(FN_ARG6);	

	UBYTE source_x = source_pos & 0xFF;
	UBYTE source_y = (source_pos >> 8) & 0xFF;
	UBYTE dest_x = dest_pos & 0xFF;
	UBYTE dest_y = (dest_pos >> 8) & 0xFF;
	UBYTE width = (wh & 0xFF) & 31;
	UBYTE height = ((wh >> 8) & 0xFF) & 31;
	UBYTE overlay_x = overlay_pos & 0xFF;
	UBYTE overlay_y = (overlay_pos >> 8) & 0xFF;
			
	scene_t scn;
    MemcpyBanked(&scn, scene_ptr, sizeof(scn), scene_bank);
	background_t bkg;
    MemcpyBanked(&bkg, scn.background.ptr, sizeof(bkg), scn.background.bank);
    const tileset_t* tileset = bkg.tileset.ptr;
	unsigned char* tilemap_ptr = bkg.tilemap.ptr;
	unsigned char* tilemap_attr_ptr = bkg.cgb_tilemap_attr.ptr;	
	
	const tileset_t* cgb_tileset = bkg.cgb_tileset.ptr;
	
	UBYTE buffer_size = sizeof(UBYTE) * width;
	for (uint8_t i = 0; i < height; i++){
		uint16_t source_offset = ((source_y + i) * (int16_t)bkg.width) + source_x;
		uint16_t dest_offset = ((dest_y + i) * image_tile_width) + dest_x;
		for (uint8_t j = 0; j < width; j++){	
			UBYTE dest_tile = ReadBankedUBYTE(image_ptr + (dest_offset + j), image_bank);	
			UBYTE source_tile = ReadBankedUBYTE(tilemap_ptr + (source_offset + j), bkg.tilemap.bank);				
			#ifdef CGB
				if (_is_CGB) {			
					
					UBYTE dest_attr = ReadBankedUBYTE(image_attr_ptr + (dest_offset + j), image_attr_bank);	
					if (copy_attributes){
						UBYTE source_attr = ReadBankedUBYTE(tilemap_attr_ptr + (source_offset + j), bkg.cgb_tilemap_attr.bank);	
						VBK_REG = 1; 
						if (copy_attributes == 1){
							set_bkg_tile_xy((dest_x + j) & 31, (dest_y + i) & 31, (dest_attr & 0x08)? (source_attr | 0x08): (source_attr & ~0x08));	
						} else if (copy_attributes == 2){
							set_win_tile_xy((overlay_x + j) & 31, (overlay_y + i) & 31, (dest_attr & 0x08)? (source_attr | 0x08): (source_attr & ~0x08));	
						}
						VBK_REG = 0;
					}					
					if (dest_attr & 0x08){ 					
						VBK_REG = 1; 
					}	
				}
			#endif
			if (cgb_tileset){
				SetBankedBkgData(dest_tile, 1, cgb_tileset->tiles + (source_tile << 4), bkg.cgb_tileset.bank);
			} else {
				SetBankedBkgData(dest_tile, 1, tileset->tiles + (source_tile << 4), bkg.tileset.bank);
			}
			#ifdef CGB
				VBK_REG = 0;
			#endif
		}
	}	
}