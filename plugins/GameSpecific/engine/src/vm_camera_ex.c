#pragma bank 255

#include <gbdk/platform.h>
#include "vm.h"
#include "vm_camera.h"

#include "actor.h"
#include "camera.h"
#include "scroll.h"
#include "game_time.h"

// VM_INVOKE handler
UBYTE vm_camera_move_to_player(void * THIS, UBYTE start, UWORD * stack_frame) OLDCALL BANKED {
	start;
	stack_frame;
    // Disable camera lock
    camera_settings &= ~(CAMERA_LOCK_FLAG);
	
	WORD target_x = PLAYER.pos.x + 128;
	WORD target_y = PLAYER.pos.y + 128;

    // Actor reached destination
    if ((camera_x == target_x) && (camera_y == target_y)) {
        camera_settings |= CAMERA_LOCK_FLAG;
        return TRUE;
    }

    // Move camera towards destination
    UBYTE x_dest = FALSE;
    if (camera_x > target_x) {
        // Move left
        camera_x -= 32;
        if (camera_x <= target_x) {
            camera_x = target_x;
            x_dest = TRUE;
        }
    } else if (camera_x < target_x) {
        // Move right
        camera_x += 32;
        if (camera_x >= target_x) {
            camera_x = target_x;
            x_dest = TRUE;
        }        
    }

    if (camera_y > target_y) {
        // Move up
        camera_y -= 32;
        if (camera_y <= target_y) {
            camera_y = target_y;
            if (x_dest) {
				camera_settings |= CAMERA_LOCK_FLAG;
                return TRUE;
            }
        }        
    } else if (camera_y < target_y) {
        // Move down
        camera_y += 32;
        if (camera_y >= target_y) {
            camera_y = target_y;
            if (x_dest) {
				camera_settings |= CAMERA_LOCK_FLAG;
                return TRUE;
            }
        }      
    }
    ((SCRIPT_CTX *)THIS)->waitable = TRUE;
	return FALSE;
}