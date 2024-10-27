#pragma bank 255

#include "scroll.h"

#include <string.h>

#include "system.h"
#include "actor.h"
#include "camera.h"
#include "data_manager.h"
#include "game_time.h"
#include "math.h"
#include "fade_manager.h"
#include "parallax.h"
#include "palette.h"
#include "meta_tiles.h"

// put submap of a large map to screen
void set_bkg_submap(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const unsigned char *map, UINT8 map_w) OLDCALL;

INT16 scroll_x;
INT16 scroll_y;
INT16 draw_scroll_x;
INT16 draw_scroll_y;
UINT16 scroll_x_max;
UINT16 scroll_y_max;
BYTE scroll_offset_x;
BYTE scroll_offset_y;
UBYTE pending_h_x, pending_h_y;
UBYTE pending_h_i;
UBYTE pending_w_x, pending_w_y;
UBYTE pending_w_i;
INT16 current_row, new_row;
INT16 current_col, new_col;
UBYTE tile_buffer[SCREEN_TILE_REFRES_W];

void scroll_init(void) BANKED {
    draw_scroll_x   = 0;
    draw_scroll_y   = 0;
    scroll_x_max    = 0;
    scroll_y_max    = 0;
    scroll_offset_x = 0;
    scroll_offset_y = 0;
	//scroll_x = 0x7FFF;
	//scroll_y = 0x7FFF;
	//game_time       = 0;
    scroll_reset();
}

void scroll_reset(void) BANKED {
    pending_w_i     = 0;
    pending_h_i     = 0;
	scroll_x = 0x7FFF;
	scroll_y = 0x7FFF;
}

void scroll_update(void) BANKED {
    INT16 x, y;
    UBYTE render = FALSE;

    x = ((camera_x >> 4) - (SCREENWIDTH >> 1));
    y = ((camera_y >> 4) - (SCREENHEIGHT >> 1));

	if (x & 0x8000u) {  // check for negative signed bit		
		x = 0u;
	} else if (x > scroll_x_max) {		
		x = scroll_x_max;
	}
	if (y & 0x8000u) {		
		y = 0u;
	} else if (y > scroll_y_max) {		
		y = scroll_y_max;
	}
    current_col = scroll_x >> 3;
    current_row = scroll_y >> 3;
    new_col = x >> 3;
    new_row = y >> 3;

    scroll_x = x;
    scroll_y = y;
    draw_scroll_x = x + scroll_offset_x;
    draw_scroll_y = y + scroll_offset_y;

    if (scroll_viewport(parallax_rows)) return;
    if (scroll_viewport(parallax_rows + 1)) return;
    scroll_viewport(parallax_rows + 2);
}

UBYTE scroll_viewport(parallax_row_t * port) BANKED {
    if (port->next_y) {
        // one of upper parallax slices
        UINT16 shift_scroll_x;
        if (port->shift == 127) {
            shift_scroll_x = 0;
        } else if (port->shift < 0) {
            shift_scroll_x = draw_scroll_x << (-port->shift);
        } else {
            shift_scroll_x = draw_scroll_x >> port->shift;
        }

        port->shadow_scx = shift_scroll_x;        
        UBYTE shift_col = shift_scroll_x >> 3;
		if (shift_col){
			// If column is +/- 1 just render next column
			if (current_col == new_col - 1) {
				// Render right column
				UBYTE x = shift_col - SCREEN_PAD_LEFT + SCREEN_TILE_REFRES_W - 1;
				scroll_load_col(x, port->start_tile, port->tile_height);
			} else if (current_col == new_col + 1) {
				// Render left column
				UBYTE x = MAX(0, shift_col - SCREEN_PAD_LEFT);
				scroll_load_col(x, port->start_tile, port->tile_height);
			} else if (current_col != new_col) {
				// If column differs by more than 1 render entire viewport
				scroll_render_rows(shift_scroll_x, 0, port->start_tile, port->tile_height);
			}  
		}
        return FALSE;   
    } else {
        // last parallax slice OR no parallax
        port->shadow_scx = draw_scroll_x;

        // If column is +/- 1 just render next column
        if (current_col == new_col - 1) {
            // Queue right column
            UBYTE x = new_col - SCREEN_PAD_LEFT + SCREEN_TILE_REFRES_W - 1;
            UBYTE y = MAX(0, MAX((new_row - SCREEN_PAD_TOP), port->start_tile));
            UBYTE full_y = MAX(0, (new_row - SCREEN_PAD_TOP));
            scroll_queue_col(x, y);
            activate_actors_in_col(x, full_y);
        } else if (current_col == new_col + 1) {
            // Queue left column
            UBYTE x = new_col - SCREEN_PAD_LEFT;
            UBYTE y = MAX(0, MAX((new_row - SCREEN_PAD_TOP), port->start_tile));
            UBYTE full_y = MAX(0, (new_row - SCREEN_PAD_TOP));
            scroll_queue_col(x, y);
            activate_actors_in_col(x, full_y);
        } else if (current_col != new_col) {
            // If column differs by more than 1 render entire screen
            scroll_render_rows(draw_scroll_x, draw_scroll_y, ((scene_LCD_type == LCD_parallax) ? port->start_tile : -SCREEN_PAD_TOP), SCREEN_TILE_REFRES_H);
            return TRUE;
        }

        // If row is +/- 1 just render next row
        if (current_row == new_row - 1) {
            // Queue bottom row
            UBYTE x = MAX(0, new_col - SCREEN_PAD_LEFT);
            UBYTE y = new_row - SCREEN_PAD_TOP + SCREEN_TILE_REFRES_H - 1;
            scroll_queue_row(x, y);
            activate_actors_in_row(x, y);
        } else if (current_row == new_row + 1) {
            // Queue top row
            UBYTE x = MAX(0, new_col - SCREEN_PAD_LEFT);
            UBYTE y = MAX(port->start_tile, new_row - SCREEN_PAD_TOP);
            scroll_queue_row(x, y);
            activate_actors_in_row(x, y);
        } else if (current_row != new_row) {
            // If row differs by more than 1 render entire screen
            scroll_render_rows(draw_scroll_x, draw_scroll_y, ((scene_LCD_type == LCD_parallax) ? port->start_tile : -SCREEN_PAD_TOP), SCREEN_TILE_REFRES_H);
            return TRUE;
        }

        if (IS_FRAME_2) {
            if (pending_h_i) scroll_load_pending_col();
            if (pending_w_i) scroll_load_pending_row();
        }

        return TRUE;
    }
}

void scroll_repaint(void) BANKED {
    scroll_reset();
    scroll_update();
}

void scroll_render_rows(INT16 scroll_x, INT16 scroll_y, BYTE row_offset, BYTE n_rows) BANKED {
    // Clear pending rows/ columns
    pending_w_i = 0;
    pending_h_i = 0;

    UBYTE x = MAX(0, (scroll_x >> 3) - SCREEN_PAD_LEFT);
    UBYTE y = MAX(0, (scroll_y >> 3) + row_offset);

    for (BYTE i = 0; i != n_rows && y != image_tile_height; ++i, y++) {
        scroll_load_row(x, y);
        activate_actors_in_row(x, y);
    }
}

void scroll_queue_row(UBYTE x, UBYTE y) BANKED {
    while (pending_w_i) {
        // If previous row wasn't fully rendered
        // render it now before starting next row        
        scroll_load_pending_row();
    }

    // Don't queue rows past image height
    if (y >= image_tile_height) {
        return;
    }

    pending_w_x = x;
    pending_w_y = y;
    pending_w_i = SCREEN_TILE_REFRES_W;
}

void scroll_queue_col(UBYTE x, UBYTE y) BANKED {
    while (pending_h_i) {
        // If previous column wasn't fully rendered
        // render it now before starting next column
        scroll_load_pending_col();
    }

    pending_h_x = x;
    pending_h_y = y;
    pending_h_i = MIN(SCREEN_TILE_REFRES_H, image_tile_height - y);
}

/* Update pending (up to 5) rows */
void scroll_load_pending_row(void) BANKED {
    UBYTE width = MIN(pending_w_i, PENDING_BATCH_SIZE);
    // DMG Row Load
	MemcpyBanked(tile_buffer, image_ptr + (UWORD)((pending_w_y * image_tile_width) + pending_w_x), width, image_bank);
	if (metatile_bank){
		for (UBYTE i = 0; i < width; i++) {
			sram_map_data[VRAM_OFFSET(pending_w_x + i, pending_w_y)] = tile_buffer[i];
			tile_buffer[i] = ReadBankedUBYTE(metatile_ptr + tile_buffer[i], metatile_bank);
		}
	}
	set_bkg_tiles(pending_w_x & 31, pending_w_y & 31, width, 1, tile_buffer);
	

#ifdef CGB
    if (_is_CGB) {  // Color Column Load
        VBK_REG = 1;		
		if (metatile_attr_bank){
			for (UBYTE i = 0; i < width; i++) {
				tile_buffer[i] = ReadBankedUBYTE(metatile_attr_ptr + sram_map_data[VRAM_OFFSET(pending_w_x + i, pending_w_y)], metatile_attr_bank);
			}
		} else {
			MemcpyBanked(tile_buffer, image_attr_ptr + (UWORD)((pending_w_y * image_tile_width) + pending_w_x), width, image_attr_bank);
		}
		set_bkg_tiles(pending_w_x & 31, pending_w_y & 31, width, 1, tile_buffer);
        VBK_REG = 0;
    }
#endif	
    pending_w_x += width;
    pending_w_i -= width;
}

void scroll_load_row(UBYTE x, UBYTE y) BANKED {
	UBYTE width = MIN(SCREEN_TILE_REFRES_W, image_tile_width);
    // DMG Row Load
	MemcpyBanked(tile_buffer, image_ptr + (UWORD)((y * image_tile_width) + x), width, image_bank);
	if (metatile_bank){
		for (UBYTE i = 0; i < width; i++) {
			sram_map_data[VRAM_OFFSET(x + i, y)] = tile_buffer[i];
			tile_buffer[i] = ReadBankedUBYTE(metatile_ptr + tile_buffer[i], metatile_bank);
		}
	}
	set_bkg_tiles(x & 31, y & 31, width, 1, tile_buffer);
	
#ifdef CGB
    if (_is_CGB) {  // Color Column Load
        VBK_REG = 1;		
		if (metatile_attr_bank){
			for (UBYTE i = 0; i < width; i++) {
				tile_buffer[i] = ReadBankedUBYTE(metatile_attr_ptr + sram_map_data[VRAM_OFFSET(x + i, y)], metatile_attr_bank);
			}
		} else {
			MemcpyBanked(tile_buffer, image_attr_ptr + (UWORD)((y * image_tile_width) + x), width, image_attr_bank);
		}
		set_bkg_tiles(x & 31, y & 31, width, 1, tile_buffer);
        VBK_REG = 0;
    }
#endif
}

void scroll_load_col(UBYTE x, UBYTE y, UBYTE height) BANKED {	
	// DMG Column Load	
	UBYTE i;
	UWORD vram_offset;
	UBYTE * column_pointer = (image_ptr + (UWORD)((y * image_tile_width) + x));
	for (i = 0; i < height; i++) {			
		tile_buffer[i] = ReadBankedUBYTE(column_pointer, image_bank);	
		column_pointer += image_tile_width;	
	}	
	if (metatile_bank){
		vram_offset = VRAM_OFFSET(x, y);		
		for (i = 0; i < height; i++) {						
			sram_map_data[vram_offset] = tile_buffer[i];
			tile_buffer[i] = ReadBankedUBYTE(metatile_ptr + (UWORD)tile_buffer[i], metatile_bank);
			vram_offset += 32;
		}	
	}
	set_bkg_tiles(x & 31, y & 31, 1, height, tile_buffer);	
#ifdef CGB
    if (_is_CGB) {  // Color Column Load
        VBK_REG = 1;
		if (metatile_attr_bank){
			vram_offset = VRAM_OFFSET(x, y);
			for (i = 0; i < height; i++) {			
				tile_buffer[i] = ReadBankedUBYTE(metatile_attr_ptr + (UWORD)sram_map_data[vram_offset], metatile_attr_bank);
				vram_offset += 32;
			}
		} else {
			column_pointer = (image_attr_ptr + (UWORD)((y * image_tile_width) + x));
			for (i = 0; i < height; i++) {
				tile_buffer[i] = ReadBankedUBYTE(column_pointer, image_attr_bank);	
				column_pointer += image_tile_width;			
			}	
		}
		set_bkg_tiles(x & 31, y & 31, 1, height, tile_buffer);
        VBK_REG = 0;
    }
#endif
}

void scroll_load_pending_col(void) NONBANKED {
	UINT8 _save = CURRENT_BANK;	
    UBYTE height = MIN(pending_h_i, PENDING_BATCH_SIZE);
	UBYTE i;
	UWORD vram_offset;
	SWITCH_ROM(image_bank);
    // DMG Column Load	
	UBYTE * column_pointer = (image_ptr + (UWORD)((pending_h_y * image_tile_width) + pending_h_x));
	for (i = 0; i < height; i++) {			
		tile_buffer[i] = *column_pointer;	
		column_pointer += image_tile_width;		
	}	
	if (metatile_bank){
		SWITCH_ROM(metatile_bank);	
		vram_offset = VRAM_OFFSET(pending_h_x, pending_h_y);		
		for (i = 0; i < height; i++) {						
			sram_map_data[vram_offset] = tile_buffer[i];
			tile_buffer[i] = *(metatile_ptr + (UWORD)tile_buffer[i]);
			vram_offset += 32;
		}	
	}
	set_bkg_tiles(pending_h_x & 31, pending_h_y & 31, 1, height, tile_buffer);	
#ifdef CGB
    if (_is_CGB) {  // Color Column Load
        VBK_REG = 1;		
		if (metatile_attr_bank){
			SWITCH_ROM(metatile_attr_bank);
			vram_offset = VRAM_OFFSET(pending_h_x, pending_h_y);
			for (i = 0; i < height; i++) {			
				tile_buffer[i] = *(metatile_attr_ptr + (UWORD)sram_map_data[vram_offset]);
				vram_offset += 32;
			}
		} else {
			SWITCH_ROM(image_attr_bank);
			column_pointer = (image_attr_ptr + (UWORD)((pending_h_y * image_tile_width) + pending_h_x));
			for (i = 0; i < height; i++) {
				tile_buffer[i] = *column_pointer;	
				column_pointer += image_tile_width;			
			}	
		}
		set_bkg_tiles(pending_h_x & 31, pending_h_y & 31, 1, height, tile_buffer);
        VBK_REG = 0;
    }
#endif
    pending_h_y += height;
    pending_h_i -= height;	
	 SWITCH_ROM(_save);
}

