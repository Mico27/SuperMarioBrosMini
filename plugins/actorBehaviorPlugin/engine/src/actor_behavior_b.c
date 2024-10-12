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

UWORD check_collision_b(UWORD start_x, UWORD start_y, bounding_box_t *bounds, col_check_dir_e check_dir) BANKED{
    switch (check_dir) {
        case CHECK_DIR_LEFT:  // Check left (bottom left)
            col_tx = (((start_x >> 4) + bounds->left) >> 3);
            col_ty = (((start_y >> 4) + bounds->bottom) >> 3);
            if (tile_at(col_tx, col_ty) & COLLISION_RIGHT) {
                return ((col_tx + 1) << 7) - (bounds->left << 4);
            }
            return start_x;
        case CHECK_DIR_RIGHT:  // Check right (bottom right)
            col_tx = (((start_x >> 4) + bounds->right) >> 3);
            col_ty = (((start_y >> 4) + bounds->bottom) >> 3);
            if (tile_at(col_tx, col_ty) & COLLISION_LEFT) {
                return (col_tx << 7) - ((bounds->right + 1) << 4);
            }
            return start_x;
        case CHECK_DIR_UP:  // Check up (middle up)
            col_ty = (((start_y >> 4) + bounds->top) >> 3);
            col_tx = (((start_x >> 4) + ((bounds->left + bounds->right) >> 1)) >> 3);
            if (tile_at(col_tx, col_ty) & COLLISION_BOTTOM) {
                return ((col_ty + 1) << 7) - ((bounds->top) << 4);
            }
            return start_y;
        case CHECK_DIR_DOWN:  // Check down (right bottom and left bottom)
            col_ty = (((start_y >> 4) + bounds->bottom) >> 3);
            col_tx = (((start_x >> 4) + bounds->left) >> 3);
            if (tile_at(col_tx, col_ty) & COLLISION_TOP) {
                return ((col_ty) << 7) - ((bounds->bottom + 1) << 4);
            }			
			col_tx = (((start_x >> 4) + bounds->right) >> 3);
			if (tile_at(col_tx, col_ty) & COLLISION_TOP) {
                return ((col_ty) << 7) - ((bounds->bottom + 1) << 4);
            }
            return start_y;
    }
    return start_x;
}

void apply_gravity_b(UBYTE actor_idx) BANKED {
	actor_vel_y[actor_idx] += (plat_grav >> 8);
	actor_vel_y[actor_idx] = MIN(actor_vel_y[actor_idx], (plat_max_fall_vel >> 8));
}

void apply_water_gravity_b(UBYTE actor_idx) BANKED {
	actor_vel_y[actor_idx] += (plat_grav >> 11);
	actor_vel_y[actor_idx] = MIN(actor_vel_y[actor_idx], (plat_max_fall_vel >> 11));
}

void apply_velocity_b(UBYTE actor_idx, actor_t * actor) BANKED {
	//Apply velocity
	WORD new_y =  actor->pos.y + actor_vel_y[actor_idx];
	WORD new_x =  actor->pos.x + actor_vel_x[actor_idx];
	if (actor->collision_enabled){
		//Tile Collision
		actor->pos.x = check_collision_b(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
		if (actor->pos.x != new_x){
			actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
		}
		actor->pos.y = check_collision_b(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
	} else {
		actor->pos.x = new_x;
		actor->pos.y = new_y;
	}
}

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
					if (!(actor_counter_a[i] & 127)){
						actor_counter_a[i] = rand();
						UBYTE bullet_idx = actor_linked_actor_idx[i];
						if (bullet_idx != 0 && (actor_states[bullet_idx] == 0 || actor_states[bullet_idx] == 255)){
							actor_t * bullet_actor = (actors + bullet_idx);
							actor_behavior_ids[bullet_idx] = 6;
							actor_states[bullet_idx] = 0;
							if (!bullet_actor->active){
								bullet_actor->disabled = FALSE;
								activate_actor(bullet_actor);
							}
							bullet_actor->collision_enabled = true;
							load_animations(bullet_actor->sprite.ptr, bullet_actor->sprite.bank, ANIM_STATE_DEFAULT, bullet_actor->animations);
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
		case 22://Hammer bros
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_counter_a[i] = rand();
					actor_counter_a[i] = 32;
					actor_vel_x[i] = 2;
					actor_vel_y[i] = 0;					
				}
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				apply_gravity_b(i);
				apply_velocity_b(i, actor);
				//Animation
				if (PLAYER.pos.x < actor->pos.x) {
					actor_set_dir(actor, DIR_LEFT, TRUE);
				} else {
					actor_set_dir(actor, DIR_RIGHT, TRUE);
				}
				if (!(game_time & 1)){
					if (!(actor_counter_b[i] & 31)){
						actor_vel_x[i] = -actor_vel_x[i];
					}
					if (!(actor_counter_a[i] & 127)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 64){
							//jump
							actor_states[i] = 2;
							actor_vel_y[i] = -60;
						} else {
							//throw hammer
							UBYTE hammer_idx = actor_linked_actor_idx[i];								
							if (hammer_idx != 0 && (actor_states[hammer_idx] == 0 || actor_states[hammer_idx] == 255)){
								actor_t * hammer_actor = (actors + hammer_idx);
								actor_behavior_ids[hammer_idx] = 23;
								actor_states[hammer_idx] = 0;
								if (!hammer_actor->active){
									hammer_actor->disabled = FALSE;
									activate_actor(hammer_actor);
								}
								hammer_actor->collision_enabled = true;
								hammer_actor->pos.y = actor->pos.y;
								hammer_actor->pos.x = actor->pos.x;	
								actor_vel_y[hammer_idx] = -(30 + (rand() & 7));
								if (PLAYER.pos.x < actor->pos.x) {
									actor_set_dir(hammer_actor, DIR_LEFT, FALSE);									
									actor_vel_x[hammer_idx]	= -12;							
								} else {
									actor_set_dir(hammer_actor, DIR_RIGHT, FALSE);
									actor_vel_x[hammer_idx]	= 12;
								}
							}							
						}					
					}
					actor_counter_a[i]++;
					actor_counter_b[i]++;
				}
				break;
			case 2: //Jump state
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 10);
				actor_vel_y[i] = MIN(actor_vel_y[i], plat_max_fall_vel >> 8);
				//Apply velocity
				WORD new_y =  actor->pos.y + actor_vel_y[i];
				//Tile Collision				
				if ((actor->pos.y < new_y) && (actor_counter_a[i] < 32 || (actor->pos.y >> 7) > 14)){					
					actor->pos.y = check_collision_b(actor->pos.x, new_y, &actor->bounds, CHECK_DIR_DOWN);
					if (actor->pos.y != new_y){
						actor_vel_y[i] = 0;
						actor_states[i] = 1;						
					}
				} else {
					actor->pos.y = new_y;
				}
				//Animation
				if (PLAYER.pos.x < actor->pos.x) {
					actor_set_anim(actor, ANIM_JUMP_LEFT);
				} else {
					actor_set_anim(actor, ANIM_JUMP_RIGHT);
				}
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 23://Arc entity (hammer, jumping cheepcheep)
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD || (actor->pos.y >> 7) > (image_tile_height + 4)){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 11);
				actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
				//Apply velocity
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				actor->pos.x =  actor->pos.x + actor_vel_x[i];					
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;
		case 24://Bloopers
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_counter_a[i] = rand();
					actor_vel_x[i] = 0;
					actor_vel_y[i] = 0;					
				}
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				if ((actor->pos.y >> 7) > 5){
					apply_water_gravity_b(i);
				} else {
					apply_gravity_b(i);
				}
				//Apply velocity
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				actor->pos.x  =  actor->pos.x + actor_vel_x[i];
				if (!(game_time & 1)){
					if (actor_vel_x[i] < 0){
						actor_vel_x[i]++;
					} else if (actor_vel_x[i] > 0){
						actor_vel_x[i]--;
					}
					if (!(actor_counter_a[i] & 31)){
						actor_counter_a[i] = 0;	
						if (PLAYER.pos.y < actor->pos.y && (actor->pos.y >> 7) > 5) {
							actor_states[i] = 2;											
						}							
					}
					actor_counter_a[i]++;
				}
				break;
			case 2: //swim up toward player state
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				if ((actor->pos.y >> 7) > 5){
					apply_water_gravity_b(i);
				} else {
					apply_gravity_b(i);
				}
				//Apply velocity
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				actor->pos.x  =  actor->pos.x + actor_vel_x[i];
				if (!(game_time & 1)){
					if (actor_vel_x[i] < 0){
						actor_vel_x[i]++;
					} else if (actor_vel_x[i] > 0){
						actor_vel_x[i]--;
					}
					if (actor_counter_a[i] < 16){
						actor->frame = actor->frame_start + 1;
						actor_counter_a[i]++;
					} else {
						actor->frame = actor->frame_start;
						actor_vel_y[i] = -30;
						if (PLAYER.pos.x < actor->pos.x) {
							actor_vel_x[i] = -16;
						} else {
							actor_vel_x[i] = 16;
						}	
						actor_counter_a[i] = rand();
						actor_states[i] = 1;
					}
				}
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 25://Underwater cheepcheep
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_vel_y[i] = (rand() > 64)? 2: 0;
					actor_counter_a[i] = 0;
				}
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}				
				if (actor_counter_a[i] > 60){
					actor_vel_y[i] = -actor_vel_y[i];
					actor_counter_a[i] = 0;
				}				
				actor_counter_a[i]++;
				actor->pos.y =  actor->pos.y + actor_vel_y[i];	
				actor->pos.x =  actor->pos.x + actor_vel_x[i];				
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;		
		case 26://Jumping cheepcheep spawner
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_counter_a[i] = rand();
				}
				break;
			case 1: //Spawn state
				if (!(game_time & 1)){
					if (!(actor_counter_a[i] & 127)){
						actor_counter_a[i] = rand();
						UBYTE cheepcheep_idx = actor_linked_actor_idx[i];
						actor_t * cheepcheep_actor = (actors + cheepcheep_idx);
						if (actor_states[cheepcheep_idx] == 0 || actor_states[cheepcheep_idx] == 255){
							actor_behavior_ids[cheepcheep_idx] = 23;
							actor_states[cheepcheep_idx] = 0;
							if (!cheepcheep_actor->active){
								cheepcheep_actor->disabled = FALSE;
								activate_actor(cheepcheep_actor);
							}
							cheepcheep_actor->collision_enabled = true;
							load_animations(cheepcheep_actor->sprite.ptr, cheepcheep_actor->sprite.bank, ANIM_STATE_RED, cheepcheep_actor->animations);
							cheepcheep_actor->pos.y = image_tile_height << 7;
							cheepcheep_actor->pos.x = PLAYER.pos.x + (((rand() & 127) - 50) << 4);
							actor_vel_y[cheepcheep_idx] = -(40 + (rand() & 15));
							actor_counter_a[cheepcheep_idx] = 0;
							if ((((cheepcheep_actor->pos.x >> 4) + 8) - draw_scroll_x) < 80){
								actor_set_dir(cheepcheep_actor, DIR_RIGHT, TRUE);
								actor_vel_x[cheepcheep_idx] = (8 + (rand() & 15));
							} else {
								actor_set_dir(cheepcheep_actor, DIR_LEFT, TRUE);
								actor_vel_x[cheepcheep_idx] = -(8 + (rand() & 15));
							}
						}
					}
					actor_counter_a[i]++;
				}		
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 27://Podoboo
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
				}
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 11);
				actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
				//Apply velocity
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				
				//Animation
				if (actor_vel_y[i] < 0) {
					actor_set_dir(actor, DIR_UP, TRUE);
				} else {
					actor_set_dir(actor, DIR_DOWN, TRUE);
				}

				if ((actor->pos.y >> 7) > image_tile_height){
					actor_counter_a[i] = 0;
					actor_states[i] = 2; 
				}
				break;
			case 2: //Wait and launch state
				if (!(game_time & 1)){
					actor_counter_a[i]++;
					if (!(actor_counter_a[i] & 63)){
						actor_vel_y[i] = -52;
						actor_states[i] = 1; 
					}					
				}
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 28://Lakitu
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor->frame = actor->frame_start;
				}
				break;
			case 1: //Main state	
				if (!(game_time & 3)){
					if (actor->pos.x > PLAYER.pos.x){
						actor_set_dir(actor, DIR_LEFT, TRUE);
						if (actor_vel_x[i] > (pl_vel_x >> 8) - 16){
							actor_vel_x[i]--;
						}
					} else {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
						if (actor_vel_x[i] < (pl_vel_x >> 8) + 16){
							actor_vel_x[i]++;
						}
					}
				}
				//Apply velocity
				actor->pos.x =  actor->pos.x + actor_vel_x[i];
				if (!(game_time & 1)){					
					if (!(actor_counter_a[i] & 63)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 128){
							actor->frame = actor->frame_start + 1;
							actor_counter_b[i] = 32;
						}				
					}
					actor_counter_a[i]++;
					if (actor_counter_b[i] != 0){
						actor_counter_b[i]--;
						if (actor_counter_b[i] == 0){
							actor->frame = actor->frame_start;
							//throw spiky 1							
							UBYTE spiky_idx = actor_linked_actor_idx[i];
							if (actor_states[spiky_idx] != 0 && actor_states[spiky_idx] != 255){
								//throw spiky 2
								spiky_idx = actor_linked_actor_idx[spiky_idx];
								if (actor_states[spiky_idx] != 0 && actor_states[spiky_idx] != 255){
									//throw spiky 3
									spiky_idx = actor_linked_actor_idx[spiky_idx];
								}
							}							
							if (spiky_idx != 0 && (actor_states[spiky_idx] == 0 || actor_states[spiky_idx] == 255)){
								actor_t * spiky_actor = (actors + spiky_idx);
								actor_behavior_ids[spiky_idx] = 29;
								actor_states[spiky_idx] = 0;
								load_animations(spiky_actor->sprite.ptr, spiky_actor->sprite.bank, ANIM_STATE_TUCKED, spiky_actor->animations);
								if (!spiky_actor->active){
									spiky_actor->disabled = FALSE;
									activate_actor(spiky_actor);
								}
								spiky_actor->collision_enabled = true;
								spiky_actor->pos.y = actor->pos.y;
								spiky_actor->pos.x = actor->pos.x;	
								actor_vel_y[spiky_idx] = -24;
								if ((PLAYER.pos.x - 256) < actor->pos.x) {
									actor_set_dir(spiky_actor, DIR_LEFT, FALSE);									
									actor_vel_x[spiky_idx]	= (pl_vel_x >> 8) - 8;							
								} else {
									actor_set_dir(spiky_actor, DIR_RIGHT, FALSE);
									actor_vel_x[spiky_idx]	= (pl_vel_x >> 8) + 8;
								}
							}
						}
					}
				}
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;
		case 29: //Spikey
		switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
					}
					break;
				case 1: //falling state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD || (actor->pos.y >> 7) > image_tile_height){ 
						actor_states[i] = 255; 
						break;
					}					
					actor_vel_y[i] += (plat_grav >> 11);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
					//Apply velocity
					WORD new_y =  actor->pos.y + actor_vel_y[i];
					WORD new_x =  actor->pos.x + actor_vel_x[i];
					//Tile Collision
					actor->pos.x = check_collision_b(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
					if (actor->pos.x != new_x){
						actor_vel_x[i] = -actor_vel_x[i];
					}
					actor->pos.y = check_collision_b(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
					if (actor->pos.y != new_y){
						load_animations(actor->sprite.ptr, actor->sprite.bank, ANIM_STATE_DEFAULT, actor->animations);
						actor_states[i] = 2; 
						actor_vel_y[i] = 0;
						if (PLAYER.pos.x < actor->pos.x) {									
							actor_vel_x[i]	= -4;							
						} else {							
							actor_vel_x[i]	= 4;
						}
					}
					break;
				case 2: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD || (actor->pos.y >> 7) > image_tile_height){ 
						actor_states[i] = 255; 
						break;
					}					
					apply_gravity_b(i);
					apply_velocity_b(i, actor);
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					}
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
		break;	
		case 30://Bowser
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_counter_a[i] = 32;
					actor_vel_x[i] = 2;
					actor_vel_y[i] = 0;					
				}
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				apply_gravity_b(i);
				apply_velocity_b(i, actor);
				//Animation
				if (PLAYER.pos.x < actor->pos.x) {
					actor_set_dir(actor, DIR_LEFT, TRUE);
				} else {
					actor_set_dir(actor, DIR_RIGHT, TRUE);
				}
				if (!(game_time & 1)){
					if (!(actor_counter_b[i] & 31)){
						actor_vel_x[i] = -actor_vel_x[i];
					}
					if (!(actor_counter_a[i] & 63)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 128){
							//jump
							actor_states[i] = 2;
							actor_vel_y[i] = -40;
						} else {
							//breath fire							
							UBYTE attack_idx = actor_linked_actor_idx[i];
							if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
								//throw hammer 1
								attack_idx = actor_linked_actor_idx[attack_idx];
								if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
									//throw hammer 2
									attack_idx = actor_linked_actor_idx[attack_idx];
								}
							}		
							if (attack_idx != 0 && (actor_states[attack_idx] == 0 || actor_states[attack_idx] == 255)){
								actor_t * attack_actor = (actors + attack_idx);
								actor_states[attack_idx] = 0;
								if (!attack_actor->active){
									attack_actor->disabled = FALSE;
									activate_actor(attack_actor);
								}
								attack_actor->collision_enabled = true;
								attack_actor->pos.y = actor->pos.y;
								attack_actor->pos.x = actor->pos.x;	
								if (actor_behavior_ids[attack_idx] == 23){
									actor_vel_y[attack_idx] = -(30 + (rand() & 7));
								}
								if (PLAYER.pos.x < actor->pos.x) {
									actor_set_dir(attack_actor, DIR_LEFT, FALSE);									
									actor_vel_x[attack_idx]	= -12;							
								} else {
									actor_set_dir(attack_actor, DIR_RIGHT, FALSE);
									actor_vel_x[attack_idx]	= 12;
								}
							}								
						}					
					}
					actor_counter_a[i]++;
					actor_counter_b[i]++;
				}
				break;
			case 2: //Jump state
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 10);
				actor_vel_y[i] = MIN(actor_vel_y[i], plat_max_fall_vel >> 8);
				//Apply velocity
				WORD new_y =  actor->pos.y + actor_vel_y[i];
				//Tile Collision				
				actor->pos.y = check_collision_b(actor->pos.x, new_y, &actor->bounds, CHECK_DIR_DOWN);
				if (actor->pos.y != new_y){
					actor_vel_y[i] = 0;
					actor_states[i] = 1;						
				}
				//Animation
				if (PLAYER.pos.x < actor->pos.x) {
					actor_set_anim(actor, ANIM_JUMP_LEFT);
				} else {
					actor_set_anim(actor, ANIM_JUMP_RIGHT);
				}
				break;
			case 3: //death
				if ((actor->pos.y >> 7) > (image_tile_height + 4)){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 10);
				actor_vel_y[i] = MIN(actor_vel_y[i], plat_max_fall_vel >> 8);
				//Apply velocity
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 31://Bullet bill spawner
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_counter_a[i] = rand();
				}
				break;
			case 1: //Spawn state
				if (!(game_time & 1)){
					if (!(actor_counter_a[i] & 127)){
						actor_counter_a[i] = rand();
						//bullet 1						
						UBYTE bullet_idx = actor_linked_actor_idx[i];
						if (actor_states[bullet_idx] != 0 && actor_states[bullet_idx] != 255){
							//bullet 2	
							bullet_idx = actor_linked_actor_idx[bullet_idx];
							if (actor_states[bullet_idx] != 0 && actor_states[bullet_idx] != 255){
								//bullet 3	
								bullet_idx = actor_linked_actor_idx[bullet_idx];
							}
						}	
						if (bullet_idx != 0 && (actor_states[bullet_idx] == 0 || actor_states[bullet_idx] == 255)){
							actor_t * bullet_actor = (actors + bullet_idx);
							actor_behavior_ids[bullet_idx] = 6;
							actor_states[bullet_idx] = 0;
							if (!bullet_actor->active){
								bullet_actor->disabled = FALSE;
								activate_actor(bullet_actor);
							}
							bullet_actor->collision_enabled = true;
							bullet_actor->pos.y = ((rand() & 15) + 2) << 7;
							bullet_actor->pos.x = (draw_scroll_x + 160) << 4;
							actor_set_dir(bullet_actor, DIR_LEFT, TRUE);
							actor_vel_x[bullet_idx] = -16;
						}
					}
					actor_counter_a[i]++;
				}		
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 32://Kill flag
			for (UBYTE j = 0; j < MAX_ACTORS; j++){
				actor_t * other_actor = (actors + j);
				if (!other_actor->active || actor == other_actor){
					continue;
				}
				UBYTE other_behavior = actor_behavior_ids[j];
				if ((other_behavior > 0 && other_behavior < 11) || (other_behavior > 20 && other_behavior < 32)){
					script_execute(other_actor->script.bank, other_actor->script.ptr, 0, 1, 2);
				}
			}
			actor_behavior_ids[i] = 0;		
		break;	
		case 33://Totomesu
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_counter_a[i] = 32;
					actor_counter_b[i] = 0;
					actor_vel_x[i] = 0;
					actor_vel_y[i] = 0;					
				}
				break;
			case 1: //Main state
				current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
				if (current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				apply_gravity_b(i);
				apply_velocity_b(i, actor);
				//Animation
				if (PLAYER.pos.x < actor->pos.x) {
					actor_set_dir(actor, DIR_LEFT, actor_counter_b[i]);
				} else {
					actor_set_dir(actor, DIR_RIGHT, actor_counter_b[i]);
				}
				if (!(game_time & 1)){
					if (!(actor_counter_a[i] & 63)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 64){
							//jump
							actor_states[i] = 2;
							actor_vel_y[i] = -32;
							actor_vel_x[i] = (rand() & 15) - 8;
							actor->anim_noloop = 1;
						} else {
							//breath fire							
							UBYTE attack_idx = actor_linked_actor_idx[i];
							if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
								//breath fire
								attack_idx = actor_linked_actor_idx[attack_idx];
								if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
									//breath fire
									attack_idx = actor_linked_actor_idx[attack_idx];
								}
							}		
							if (attack_idx != 0 && (actor_states[attack_idx] == 0 || actor_states[attack_idx] == 255)){
								actor_t * attack_actor = (actors + attack_idx);
								actor_states[attack_idx] = 0;
								if (!attack_actor->active){
									attack_actor->disabled = FALSE;
									activate_actor(attack_actor);
								}
								attack_actor->collision_enabled = true;
								attack_actor->pos.y = actor->pos.y - (actor_counter_a[i] - 128);
								actor_counter_b[i] = 15;
								if (PLAYER.pos.x < actor->pos.x) {
									actor_set_dir(attack_actor, DIR_LEFT, FALSE);	
									attack_actor->pos.x = actor->pos.x - 128;	
									actor_vel_x[attack_idx]	= -12;											
								} else {
									actor_set_dir(attack_actor, DIR_RIGHT, FALSE);
									attack_actor->pos.x = actor->pos.x + 128;
									actor_vel_x[attack_idx]	= 12;		
								}
							}								
						}					
					}
					actor_counter_a[i]++;
					if (actor_counter_b[i] > 0){
						actor_counter_b[i]--;
					}
				}
				break;
			case 2: //Jump state
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 11);
				actor_vel_y[i] = MIN(actor_vel_y[i], plat_max_fall_vel >> 9);
				//Apply velocity
				WORD new_y =  actor->pos.y + actor_vel_y[i];
				WORD new_x =  actor->pos.x + actor_vel_x[i];
				//Tile Collision				
				actor->pos.x = check_collision_b(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
				if (actor->pos.x != new_x){
					actor_vel_x[i] = -actor_vel_x[i];
				}
				actor->pos.y = check_collision_b(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
				if (actor->pos.y < new_y){
					actor_vel_y[i] = 0;
					actor_vel_x[i] = 0;
					actor->anim_noloop = 0;
					actor_states[i] = 1;						
				}
				//Animation
				if (PLAYER.pos.x < actor->pos.x) {					
					actor_set_anim(actor, ANIM_JUMP_LEFT);
				} else {
					actor_set_anim(actor, ANIM_JUMP_RIGHT);
				}
				break;
			case 3: //death
				if ((actor->pos.y >> 7) > (image_tile_height + 4)){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 10);
				actor_vel_y[i] = MIN(actor_vel_y[i], plat_max_fall_vel >> 8);
				//Apply velocity
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 34: //Pokey
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 4; 
						actor_vel_y[i] = 0;
						actor_vel_x[i] = -4;
					}
					break;
				case 1: //Main state
				case 2:
				case 3:
				case 4:
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					apply_gravity_b(i);
					apply_velocity_b(i, actor);
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					break;
				case 255: //Deactivate
					deactivate_actor(actor);
					break;
			}		
		break;
		case 17: //Hit block bump
		switch(actor_states[i]){
			case 0:
				//Actor Collision	
				for (UBYTE j = 1; j < MAX_ACTORS; j++){
					actor_t * hit_actor = (actors + j);
					if (!hit_actor->active || hit_actor == actor || !hit_actor->collision_enabled){
						continue;
					}
					if (bb_intersects(&actor->bounds, &actor->pos, &hit_actor->bounds, &hit_actor->pos)) {
						
						if ((hit_actor->pos.x > actor->pos.x && actor_vel_x[j] < 0) ||
							(hit_actor->pos.x < actor->pos.x && actor_vel_x[j] > 0)) {								
							actor_vel_x[j] = -actor_vel_x[j];
						}
						actor_vel_y[j] = -60;
						if (hit_actor->script.bank){
							script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 2);
						}
					}
				}
				actor_states[i] = 255;
				break;
		}
		break;		
	}			
}