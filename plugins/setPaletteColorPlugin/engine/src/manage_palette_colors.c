#pragma bank 255

#include <gbdk/platform.h>
#include "system.h"
#include "vm.h"
#include "gbs_types.h"
#include "palette.h"
#include "math.h"

void set_palette_colors(SCRIPT_CTX * THIS) OLDCALL BANKED {
	int16_t palettes = *(int16_t*)VM_REF_TO_PTR(FN_ARG0);
	int16_t color0 = *(int16_t*)VM_REF_TO_PTR(FN_ARG1);
	int16_t color1 = *(int16_t*)VM_REF_TO_PTR(FN_ARG2);
	int16_t color2 = *(int16_t*)VM_REF_TO_PTR(FN_ARG3);
	int16_t color3 = *(int16_t*)VM_REF_TO_PTR(FN_ARG4);

	UBYTE palette_from_idx = palettes & 7;
	UBYTE is_sprite = (palettes >> 3) & 1;
	UBYTE is_dmg = (palettes >> 4) & 1;
	
	//UBYTE r1 = color1 & 31;
	//UBYTE g1 = (color1 >> 5) & 31;
	//UBYTE b1 = (color1 >> 10) & 31;
	if (is_dmg) {
		UBYTE DMGPal = DMG_PALETTE(color0 & 3, color1 & 3, color2 & 3, color3 & 3);
        switch (palette_from_idx & 1) {
            case 0:
                if (is_sprite) {
                    DMG_palette[1] = DMGPal;
                    OBP0_REG = DMGPal;
                } else {
					DMG_palette[0] = DMGPal;
                    BGP_REG = DMGPal;
				}
                break;
            case 1:
                if (is_sprite) {
                    DMG_palette[2] = DMGPal;
                    OBP1_REG = DMGPal;
                }
                break;
        }
	} else {
		if (is_sprite){
			SprPalette[palette_from_idx].c0 = color0;
			SprPalette[palette_from_idx].c1 = color1;
			SprPalette[palette_from_idx].c2 = color2;
		} else {
			BkgPalette[palette_from_idx].c0 = color0;
			BkgPalette[palette_from_idx].c1 = color1;
			BkgPalette[palette_from_idx].c2 = color2;
			BkgPalette[palette_from_idx].c3 = color3;
		}
#ifdef CGB
		if (_is_CGB) {
			if (is_sprite){
				set_sprite_palette(palette_from_idx, 1, (void *)(SprPalette + palette_from_idx));
			} else {
				set_bkg_palette(palette_from_idx, 1, (void *)(BkgPalette + palette_from_idx));
			}
		}
#endif
	}
}