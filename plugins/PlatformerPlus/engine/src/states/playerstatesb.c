#pragma bank 255

#include "data/states_defines.h"
#include "states/platform.h"
#include "actor.h"
#include "camera.h"
#include "collision.h"
#include "data_manager.h"
#include "game_time.h"
#include "input.h"
#include "math.h"
#include "scroll.h"
#include "trigger.h"
#include "vm.h"
#include "states/playerstates.h"
#include "states/playerstatesb.h"
#include "meta_tiles.h"

#ifndef INPUT_PLATFORM_JUMP
#define INPUT_PLATFORM_JUMP        INPUT_A
#endif
#ifndef INPUT_PLATFORM_RUN
#define INPUT_PLATFORM_RUN         INPUT_B
#endif
#ifndef INPUT_PLATFORM_INTERACT
#define INPUT_PLATFORM_INTERACT    INPUT_A
#endif
#ifndef PLATFORM_CAMERA_DEADZONE_Y
#define PLATFORM_CAMERA_DEADZONE_Y 16
#endif

#define ANIM_STATE_DEFAULT 0
#define ANIM_STATE_CLIMB 23
#define ANIM_STATE_PIPETRANSITION 24

void start_level_init(void) BANKED {
	switch (entry_mode){
		case 0:
			//Grounded start
			que_state = GROUND_INIT;
		break;
		case 1:
			//Pipe
			PLAYER.pos.x = PLAYER.pos.x + 64; //horizontal offset to middle of pipe
			entry_y = PLAYER.pos.y;
			PLAYER.pos.y = PLAYER.pos.y + 128;//Offset below entry to animate coming out of pipe
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_PIPETRANSITION, PLAYER.animations); //set pipe transition animation
			que_state = START_LEVEL_STATE;
		break;
		case 2:
			//Vine
			entry_y = PLAYER.pos.y;
			PLAYER.pos.y = PLAYER.pos.y + 256; //Offset below entry to animate climbing the vine
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_CLIMB, PLAYER.animations); //set climbing animation
			actor_set_dir(&PLAYER, DIR_LEFT, TRUE); //move to the left of the vine
			que_state = START_LEVEL_STATE;
		break;
		case 3:
			//Sky
			que_state = FALL_INIT;
		break;
	}	
}

void start_level_state(void) BANKED {
	switch (entry_mode){
		case 1:
			//Pipe
			if (PLAYER.pos.y > entry_y) {
				PLAYER.pos.y -= 16;
			} else {				
				load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_DEFAULT, PLAYER.animations); //reset to default animation
				que_state = GROUND_INIT;
			}
		break;
		case 2:
			//Vine
			if (PLAYER.pos.y > entry_y) {
				PLAYER.pos.y -= 16;
			} else {
				actor_set_dir(&PLAYER, DIR_RIGHT, FALSE);
				que_state = VINE_INIT;
			}
		break;
	}	
}