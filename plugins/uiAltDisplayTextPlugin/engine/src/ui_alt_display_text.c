#pragma bank 255

#include <string.h>

#include "system.h"
#include "ui.h"
#include "game_time.h"
#include "bankdata.h"
#include "input.h"
#include "math.h"
#include "shadow.h"
#include "music_manager.h"
#include "actor.h"
#include "camera.h"
#include "data_manager.h"
#include "fade_manager.h"
#include "parallax.h"
#include "scroll.h"
#include "projectiles.h"
#include "vm.h"
#include "data/char_tileset_mapping.h"

UBYTE ui_alt_current_text_speed;
UBYTE ui_alt_text_drawn;

// char printer internals
static UBYTE * ui_alt_text_ptr;
static UBYTE * ui_alt_dest_ptr;
static UBYTE * ui_alt_dest_base;


inline void ui_alt_set_tile(UBYTE * addr, UBYTE tile) {
#ifdef CGB
    if (_is_CGB) {
        VBK_REG = 1;
        set_vram_byte(addr, overlay_priority | (text_palette & 0x07u));
        VBK_REG = 0;
    }
#endif
    set_vram_byte(addr, tile);
}

UBYTE ui_alt_draw_text_buffer_char(void) BANKED {
    static UBYTE current_text_ff_joypad, current_text_draw_speed;

    if (ui_alt_text_ptr == 0) {
        // set the delay mask
        ui_alt_current_text_speed = ui_time_masks[text_draw_speed];
        // save font and color global properties
        current_text_ff_joypad  = text_ff_joypad;
        current_text_draw_speed = text_draw_speed;
        // reset to first line
        // current char pointer
        ui_alt_text_ptr = ui_text_data;
        // VRAM destination
        if ((text_options & TEXT_OPT_PRESERVE_POS) == 0) {
            ui_alt_dest_base = text_render_base_addr;                  // gotoxy(0,0)
            // initialize current pointer with corrected base value
            ui_alt_dest_ptr = ui_alt_dest_base;
        }
    }

    // normally runs once, but if control code encountered, then process them until printable symbol or terminator
    while (TRUE) {
        switch (*ui_alt_text_ptr) {
            case 0x00: {
                ui_alt_text_ptr = 0;
                ui_alt_text_drawn = TRUE;
                text_ff_joypad = current_text_ff_joypad;
                text_draw_speed = current_text_draw_speed;
                return FALSE;
            }
            case 0x01:
                // set text speed
                text_draw_speed = (*(++ui_alt_text_ptr) - 1u) & 0x07u;
                ui_alt_current_text_speed = ui_time_masks[text_draw_speed];
                break;
            case 0x02: {
                break;
            }
            case 0x03:
                // gotoxy
                ui_alt_dest_ptr = ui_alt_dest_base = text_render_base_addr + (*++ui_alt_text_ptr - 1u) + (*++ui_alt_text_ptr - 1u) * 32u;
                break;
            case 0x04: {
                // relative gotoxy
                BYTE dx = (BYTE)(*++ui_alt_text_ptr);
                if (dx > 0) dx--;
                BYTE dy = (BYTE)(*++ui_alt_text_ptr);
                if (dy > 0) dy--;
                ui_alt_dest_base = ui_alt_dest_ptr += dx + dy * 32u;
                break;
            }
            case 0x06:
                // wait for input cancels fast forward
                if (text_ff) {
                    text_ff = FALSE;
                    INPUT_RESET;
                }
                text_ff_joypad = FALSE;
                // point to the button mask
                ui_alt_text_ptr++;
                // if high speed then skip waiting
                if (text_draw_speed) {
                    // wait for key press (parameter is a mask)
                    if (INPUT_PRESSED(*ui_alt_text_ptr)) {
                        // mask matches
                        text_ff_joypad = current_text_ff_joypad;
                        INPUT_RESET;
                    } else {
                        // go back to 0x06 control code
                        ui_alt_text_ptr--;
                        ui_alt_current_text_speed = 0;
                        return FALSE;
                    }
                }
                ui_alt_current_text_speed = ui_time_masks[text_draw_speed];
                break;
            case 0x07:
                break;
            case 0x08:
                break;
            case 0x09:
                break;
            case '\n':  // 0x0a
                // carriage return
                ui_alt_dest_ptr = ui_alt_dest_base += 32u;
                break;
            case 0x0b:
			#ifdef CGB
                text_palette = (*++ui_alt_text_ptr & 0x07);
			#endif
                break;
            case '\r':  // 0x0d
                // line feed
                ui_alt_dest_ptr = ui_alt_dest_base += 32u;
                break;
            case 0x05:
                // escape symbol
                ui_alt_text_ptr++;
                // fall down to default
            default:
				UBYTE tile = ReadBankedUBYTE(char_tileset_mapping + (*ui_alt_text_ptr) , BANK(char_tileset_mapping));
                ui_alt_set_tile(ui_alt_dest_ptr, tile);
				ui_alt_dest_ptr++;
                ui_alt_text_ptr++;
                return TRUE;
        }
        ui_alt_text_ptr++;
    }
}

void ui_alt_display_text(SCRIPT_CTX * THIS) OLDCALL BANKED {
	THIS;
    ui_alt_text_drawn = FALSE;
    // all drawn - nothing to do
	do {
		ui_alt_draw_text_buffer_char();		
	} while (!ui_alt_text_drawn);    
}

void ui_alt_display_dialogue(SCRIPT_CTX * THIS) OLDCALL BANKED {
	THIS;
	INPUT_RESET;
    ui_alt_text_drawn = text_ff = FALSE;
	ui_alt_current_text_speed = 0;
	UBYTE play_sound, speed_wait = FALSE;
    // all drawn - nothing to do
	do {
		// too fast - wait
		if ((text_ff_joypad) && (INPUT_A_OR_B_PRESSED)) {
			text_ff = TRUE;
		} else {
			if (game_time & ui_alt_current_text_speed) {
				speed_wait = TRUE;
			}
		}
		// render next char
		if (!speed_wait){
			do {
				play_sound = ui_alt_draw_text_buffer_char();
			} while (((text_ff) || (text_draw_speed == 0)) && (!ui_alt_text_drawn));
			// play sound
			if ((play_sound) && (text_sound_bank != SFX_STOP_BANK)) music_play_sfx(text_sound_bank, text_sound_data, text_sound_mask, MUSIC_SFX_PRIORITY_NORMAL);
		}
		speed_wait = FALSE;
		toggle_shadow_OAM();
		camera_update();
		scroll_update();
		actors_update();
		projectiles_render();
		activate_shadow_OAM();
	
		game_time++;
		wait_vbl_done();
		input_update();
		
	} while (!ui_alt_text_drawn);    
}