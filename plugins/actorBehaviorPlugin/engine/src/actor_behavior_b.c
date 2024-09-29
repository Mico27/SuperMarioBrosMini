#pragma bank 255

#include <string.h>
#include <stdlib.h>
#include <gbdk/platform.h>
#include <rand.h>
#include "system.h"
#include "vm.h"
#include "gbs_types.h"
#include "events.h"
#include "input.h"
#include "math.h"
#include "actor.h"
#include "scroll.h"
#include "game_time.h"
#include "actor_behavior.h"
#include "actor_behavior_b.h"
#include "states/platform.h"
#include "states/playerstates.h"
#include "data/states_defines.h"
#include "meta_tiles.h"
#include "collision.h"
#include "data_manager.h"
#include "data/game_globals.h"

void actor_behavior_update_b(UBYTE i, actor_t * actor) BANKED {
	switch(actor_behavior_ids[i]){	
		case 21://Cannon
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_counter_a[i] = rand();
				}
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				if (!(game_time & 1)){
					if (!(actor_counter_a[i] & 255)){
						actor_counter_a[i] = rand();
						UBYTE bullet_idx = actor_linked_actor_idx[i];
						actor_t * bullet_actor = (actors + bullet_idx);
						if (actor_states[bullet_idx] == 0 || actor_states[bullet_idx] == 255){
							actor_behavior_ids[bullet_idx] = 6;
							actor_states[bullet_idx] = 0;
							if (!bullet_actor->active){
								bullet_actor->disabled = FALSE;
								activate_actor(bullet_actor);
							}
							bullet_actor->collision_enabled = true;
							bullet_actor->pos.y = actor->pos.y;
							if (PLAYER.pos.x < actor->pos.x) {
								actor_set_dir(bullet_actor, DIR_LEFT, FALSE);
								bullet_actor->pos.x = ((actor->pos.x >> 7) - 1) << 7;	
								actor_vel_x[bullet_idx]	= -16;							
							} else {
								actor_set_dir(bullet_actor, DIR_RIGHT, FALSE);
								bullet_actor->pos.x = ((actor->pos.x >> 7) + 1) << 7;	
								actor_vel_x[bullet_idx]	= 16;
							}
						}
					}
					actor_counter_a[i]++;
				}
				break;
			case 255: //Deactivate
				actor_counter_a[i] = 0;
				deactivate_actor(actor);
				break;
		}		
		break;	
	}			
}