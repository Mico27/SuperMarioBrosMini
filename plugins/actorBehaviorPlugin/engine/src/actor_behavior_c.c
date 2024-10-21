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
#include "actor_behavior_c.h"
#include "states/platform.h"
#include "states/playerstates.h"
#include "data/states_defines.h"
#include "meta_tiles.h"
#include "collision.h"
#include "data_manager.h"
#include "data/game_globals.h"

const BYTE spring_bb_top_lookup[] = { -8, -4, 0, -4, -8 };

UWORD check_collision_c(UWORD start_x, UWORD start_y, bounding_box_t *bounds, col_check_dir_e check_dir) BANKED{    
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

void actor_behavior_update_c(UBYTE i, actor_t * actor) BANKED {
	switch(actor_behavior_ids[i]){				
		case 35://Dragonzamazu
		switch(actor_states[i]){
			case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_vel_y[i] = 8;
						actor_vel_x[i] = 0;
					}
					break;
			case 1: //Move up state
				if (!(game_time & 3)){
					actor_vel_y[i] = MAX(actor_vel_y[i]--, -8);						
					if (actor->pos.y < 1280){
						actor_states[i] = 2;
					}
				}					
				actor->pos.y = actor->pos.y + actor_vel_y[i];					
				goto dragonzamazu_mainstate;
			case 2: //Move down state
				if (!(game_time & 3)){
					actor_vel_y[i] = MIN(actor_vel_y[i]++, 8);
					if (actor->pos.y > 1536){
						actor_states[i] = 1;
					}	
				}					
				actor->pos.y = actor->pos.y + actor_vel_y[i];	
			dragonzamazu_mainstate: //Main state
				//Animation
				if (PLAYER.pos.x < actor->pos.x) {
					actor_set_dir(actor, DIR_LEFT, actor_counter_b[i]);
				} else {
					actor_set_dir(actor, DIR_RIGHT, actor_counter_b[i]);
				}
				if (!(game_time & 1)){
					if (!(actor_counter_a[i] & 63)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 128 && (PLAYER.pos.x < actor->pos.x)){
							//bullet bill 1							
							UBYTE attack_idx = actor_linked_actor_idx[i];
							if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
								//bullet bill 2
								attack_idx = actor_linked_actor_idx[attack_idx];
								if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
									//bullet bill 3
									attack_idx = actor_linked_actor_idx[attack_idx];
								}
							}		
							if (attack_idx != 0 && (actor_states[attack_idx] == 0 || actor_states[attack_idx] == 255)){
								actor_t * attack_actor = (actors + attack_idx);
								actor_behavior_ids[attack_idx] = 6;
								actor_states[attack_idx] = 0;
								if (!attack_actor->active){
									attack_actor->disabled = FALSE;
									activate_actor(attack_actor);
								}
								attack_actor->collision_enabled = true;
								attack_actor->pos.x = actor->pos.x;
								attack_actor->pos.y = actor->pos.y - 256;
								actor_counter_b[i] = 15;
								actor_set_dir(attack_actor, DIR_LEFT, FALSE);	
								actor_vel_x[attack_idx]	= -12;	
							}								
						}					
					}
					actor_counter_a[i]++;
					if (actor_counter_b[i] > 0){
						actor_counter_b[i]--;
					}
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
				actor->collision_enabled = false;
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;	
		case 36://Hiyoihoi
		switch(actor_states[i]){
			case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
					}
					break;
			case 1: //Main state
				//Animation				
				if (!(game_time & 1)){
					actor_set_dir(actor, DIR_LEFT, actor_counter_b[i]);
					if (!(actor_counter_a[i] & 63)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 128 && (PLAYER.pos.x < actor->pos.x)){
							//boulder 1							
							UBYTE attack_idx = actor_linked_actor_idx[i];
							if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
								//boulder 2
								attack_idx = actor_linked_actor_idx[attack_idx];
								if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
									//boulder 3
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
								attack_actor->pos.x = actor->pos.x;
								attack_actor->pos.y = actor->pos.y - 256;
								actor_counter_b[i] = 15;
								actor_set_dir(attack_actor, DIR_LEFT, TRUE);	
								actor_vel_x[attack_idx]	= (rand() & 7) - 12;	
								actor_vel_y[attack_idx]	= (rand() & 15) - 30;
							}								
						}					
					}
					actor_counter_a[i]++;
					if (actor_counter_b[i] > 0){
						actor_counter_b[i]--;
					}
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
				actor->collision_enabled = false;
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;
		case 37://Boulder
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
				break;
			case 1: //Main state
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
				actor->pos.x = check_collision_c(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
				if (actor->pos.x != new_x){
					actor_vel_x[i] = -actor_vel_x[i];
				}
				actor->pos.y = check_collision_c(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
				if (actor->pos.y < new_y){
					actor_vel_y[i] = -20;
					if (actor_vel_x[i] == 0){
						actor_vel_x[i] = -8;
					}
				} else if (actor->pos.y > new_y){
					actor_vel_y[i] = 0;
				}
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
		case 38://Alt projectile Lakitu
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
							//throw item 1							
							UBYTE item_idx = actor_linked_actor_idx[i];
							if (actor_states[item_idx] != 0 && actor_states[item_idx] != 255){
								//throw item 2
								item_idx = actor_linked_actor_idx[item_idx];
								if (actor_states[item_idx] != 0 && actor_states[item_idx] != 255){
									//throw item 3
									item_idx = actor_linked_actor_idx[item_idx];
								}
							}							
							if (item_idx != 0 && (actor_states[item_idx] == 0 || actor_states[item_idx] == 255)){
								actor_t * item_actor = (actors + item_idx);
								actor_states[item_idx] = 0;
								if (!item_actor->active){
									item_actor->disabled = FALSE;
									activate_actor(item_actor);
								}
								item_actor->collision_enabled = true;
								item_actor->pos.y = actor->pos.y;
								item_actor->pos.x = actor->pos.x;	
								actor_vel_y[item_idx] = -24;
								if ((PLAYER.pos.x - 256) < actor->pos.x) {
									actor_set_dir(item_actor, DIR_LEFT, FALSE);									
									actor_vel_x[item_idx]	= (pl_vel_x >> 8) - 8;							
								} else {
									actor_set_dir(item_actor, DIR_RIGHT, FALSE);
									actor_vel_x[item_idx]	= (pl_vel_x >> 8) + 8;
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
		case 39://Tatanga
		switch(actor_states[i]){
			case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_vel_y[i] = 8;
						actor_vel_x[i] = 0;
					}
					break;
			case 1: //Move up state
				if (!(game_time & 3)){
					actor_vel_y[i] = MAX(actor_vel_y[i]--, -8);						
					if (actor->pos.y < 1280){
						actor_states[i] = 2;
					}
				}					
				actor->pos.y = actor->pos.y + actor_vel_y[i];					
				goto tatanga_mainstate;
			case 2: //Move down state
				if (!(game_time & 3)){
					actor_vel_y[i] = MIN(actor_vel_y[i]++, 8);
					if (actor->pos.y > 1536){
						actor_states[i] = 1;
					}	
				}					
				actor->pos.y = actor->pos.y + actor_vel_y[i];	
			tatanga_mainstate: //Main state
				//Animation
				if (!(game_time & 3)){
					if (actor->pos.x > PLAYER.pos.x){
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					}
					if ((actor->pos.x >> 4) > draw_scroll_x + ((script_memory[VAR_BOWSER_COUNTER] == 1)? 144: 128)){
						if (actor_vel_x[i] > 16){
							actor_vel_x[i] = 0;
						}
						if (actor_vel_x[i] > -16){
							actor_vel_x[i]--;
						}
					} else {
						if (actor_vel_x[i] < 16){
							actor_vel_x[i]++;
						}
					}
				}
				if ((script_memory[VAR_BOWSER_COUNTER] == 1) && (actor->pos.x >> 4) < draw_scroll_x + 120){
					actor_vel_x[i] = 24;
				}
				actor->pos.x =  actor->pos.x + actor_vel_x[i];
				
				if (!(game_time & 1)){
					if (!(actor_counter_a[i] & 63)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 128 && (PLAYER.pos.x < actor->pos.x)){
							//Attack 1							
							UBYTE attack_idx = actor_linked_actor_idx[i];
							if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255 || rand() > 128){
								//Attack 2
								attack_idx = actor_linked_actor_idx[attack_idx];								
							}		
							if (attack_idx != 0 && (actor_states[attack_idx] == 0 || actor_states[attack_idx] == 255)){
								actor_t * attack_actor = (actors + attack_idx);
								actor_states[attack_idx] = 0;
								if (!attack_actor->active){
									attack_actor->disabled = FALSE;
									activate_actor(attack_actor);
								}
								attack_actor->collision_enabled = true;
								attack_actor->pos.x = actor->pos.x;
								attack_actor->pos.y = actor->pos.y;
								actor_set_dir(attack_actor, DIR_LEFT, FALSE);	
								actor_vel_x[attack_idx]	= 0;	
							}								
						}					
					}
					actor_counter_a[i]++;
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
				actor->collision_enabled = false;
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;
		case 40: //Wario
		switch(actor_states[i]){
			case 0:
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
					actor_vel_y[i] = 0;
					actor_vel_x[i] = 0;
					actor_counter_a[i] = rand();
				}
				break;
			case 1: //Grounded
				actor_vel_y[i] += (plat_grav >> 8);
				actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));	
				actor_states[i] = 4;
				//animation
				if (actor_vel_x[i] > 0){
					actor_set_dir(actor, DIR_RIGHT, TRUE);
				} else if (actor_vel_x[i] < 0){
					actor_set_dir(actor, DIR_LEFT, TRUE);
				} else if (actor->pos.x < PLAYER.pos.x){
					actor_set_dir(actor, DIR_RIGHT, FALSE);
				} else {
					actor_set_dir(actor, DIR_LEFT, FALSE);
				}			
				if (!(game_time & 3) && script_memory[VAR_BOWSER_COUNTER] == 2){
					if (!(actor_counter_a[i] & 31)){
						if (rand() < 200){
							actor_states[i] = 2;
							break;
						} else {
							actor_vel_x[i] = -actor_vel_x[i];
							actor_counter_a[i] = rand();
						}
					}
					actor_counter_a[i]++;
				}
				goto wario_mainstate;
			case 2: //init jump
				actor->anim_noloop = TRUE;
				actor_vel_y[i] = -30;
				actor_counter_a[i] = 10;
				actor_states[i] = 3;
				if (actor_vel_x[i] > 0){
					actor_set_anim(actor, ANIM_JUMP_RIGHT);
				} else {
					actor_set_anim(actor, ANIM_JUMP_LEFT);
				}
			case 3: //Jump				
				if (actor_counter_a[i] !=0){
					actor_vel_y[i] -= 1;
					actor_counter_a[i] -=1;
				} else if (actor_vel_y[i] < 0){
					actor_vel_y[i] += (plat_hold_grav >> 8);
				} else if (actor_vel_y[i] >= 0){
					actor->anim_noloop = FALSE;
					actor_states[i] = 4;
					actor_counter_a[i] = 0;
					actor_vel_y[i] += (plat_grav >> 8);
				} else {
					actor_vel_y[i] += (plat_grav >> 8);
				}
				goto wario_mainstate;
			case 4: //Falling
				actor_vel_y[i] += (plat_grav >> 8);
				actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));	
			wario_mainstate: //Main state				
				//Apply velocity
				WORD new_y =  actor->pos.y + actor_vel_y[i];
				WORD new_x =  actor->pos.x + actor_vel_x[i];
				//Tile Collision
				actor->pos.x = check_collision_c(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
				if (script_memory[VAR_BOWSER_COUNTER] == 2 && (actor->pos.x != new_x || (actor->pos.x >> 4) < draw_scroll_x || (actor->pos.x >> 4) > draw_scroll_x + 144)){
					actor_vel_x[i] = -actor_vel_x[i];
				}
				actor->pos.y = check_collision_c(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
				if (actor->pos.y < new_y && actor_states[i] != 1){ //grounded
					actor_states[i] = 1;
					actor_counter_a[i] = rand();
				} else if (actor->pos.y > new_y){
					actor_vel_y[i] = 0;
					UBYTE tile_id = sram_map_data[VRAM_OFFSET(col_tx, col_ty)];	
					switch(tile_id){
						case 5://coin block
						case 7://brick	
						case 152://multi coin brick
						case 153://powerup brick
						case 154://star brick
						case 155://1up brick
						case 156://powerup block	
						case 157://beanstalk block
						case 158://star block
						case 159://1up block
						case 169://beanstalk brick
						case 171://egg brick
						case 172://egg block
						if(specific_events[HIT_BLOCK_EVENT].script_addr != 0){
							script_memory[VAR_HITBLOCKID] = tile_id;
							script_memory[VAR_HITBLOCKX] = col_tx;
							script_memory[VAR_HITBLOCKY] = col_ty;
							script_memory[VAR_HITBLOCKSOURCE] = i;
							script_execute(specific_events[HIT_BLOCK_EVENT].script_bank, specific_events[HIT_BLOCK_EVENT].script_addr, 0, 0);
						}
						break;
						default:
						if (actor->script.bank){
							script_execute(actor->script.bank, actor->script.ptr, 0, 1, 8);
						}
						break;
					}
				}
				break;
			case 5: //static
				break;
			case 6: //hurt init
				actor_counter_a[i] = (rand() & 15) + 15;
				actor_states[i] = 7; 
			case 7: //hurt				
				if (!(game_time & 3)){
					actor_counter_a[i]--;					
					if (actor_counter_a[i] <= 0){
						actor_states[i] = 1;
						load_animations(actor->sprite.ptr, actor->sprite.bank, STATE_DEFAULT, actor->animations);				
					}				
				}				
				break;
			case 8: //death
				if ((actor->pos.y >> 7) > (image_tile_height + 4)){ 
					actor_states[i] = 255; 
					break;
				}
				actor_vel_y[i] += (plat_grav >> 10);
				actor_vel_y[i] = MIN(actor_vel_y[i], plat_max_fall_vel >> 8);
				//Apply velocity
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				actor->collision_enabled = false;
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;
		case 14: //Spring
		switch(actor_states[i]){
			case 0:
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
					actor_states[i] = 1; 
				}
				break;
			case 1: //Idle state
				if (actor_attached && last_actor == actor) {//start springing on player attach
					actor_counter_a[i] = 0;
					actor_states[i] = 2; 
				}					
				break;
			case 2: //Springing state
				que_state = GROUND_STATE;
				if (!actor_attached || last_actor != actor) {
					actor_counter_a[i] = 0;
					actor_states[i] = 1;
					actor->frame = actor->frame_start;
					break;						
				}
				if (!(game_time & 1)){
					actor_counter_a[i]++;
					if (actor_counter_a[i] > 4){
						actor_counter_a[i] = 0;
						actor_states[i] = 1;
						load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, STATE_DEFAULT, PLAYER.animations);
						hold_jump_val = (plat_hold_jump_max << 1); 
						actor_attached = FALSE;
						pl_vel_y = -(plat_jump_min << 1);
						jb_val = 0;
						ct_val = 0;
						enemy_bounce = 1;
						que_state = JUMP_STATE;
					}
					actor->frame = actor->frame_start + actor_counter_a[i];
					PLAYER.pos.y = (actor->pos.y + ((actor->bounds.top + spring_bb_top_lookup[actor_counter_a[i]]) << 4));
				}
				break;
			case 255:
				actor_counter_a[i] = 0;
				deactivate_actor(actor);
				break;
		}
		break;	
		case 41://DonkeyKong
		switch(actor_states[i]){
			case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
					}
					break;
			case 1: //Main state
				//Animation				
				if (!(game_time & 1)){
					actor_set_dir(actor, DIR_LEFT, actor_counter_b[i]);
					if (!(actor_counter_a[i] & 63)){	
						actor_counter_a[i] = rand();	
						if (actor_counter_a[i] < 128 && (PLAYER.pos.x < actor->pos.x)){
							//boulder 1							
							UBYTE attack_idx = actor_linked_actor_idx[i];
							if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
								//boulder 2
								attack_idx = actor_linked_actor_idx[attack_idx];
								if (actor_states[attack_idx] != 0 && actor_states[attack_idx] != 255){
									//boulder 3
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
								attack_actor->pos.x = actor->pos.x;
								attack_actor->pos.y = actor->pos.y - 256;
								actor_counter_b[i] = 15;
								actor_set_dir(attack_actor, DIR_LEFT, TRUE);	
								actor_vel_x[attack_idx]	= (rand() & 7) - 12;	
								actor_vel_y[attack_idx]	= (rand() & 15) - 30;
							}								
						}					
					}
					actor_counter_a[i]++;
					if (actor_counter_b[i] > 0){
						actor_counter_b[i]--;
					}
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
				actor->collision_enabled = false;
				break;
			case 255: //Deactivate
				deactivate_actor(actor);
				break;
		}		
		break;
		case 42://Barrel
		switch(actor_states[i]){
			case 0: //Init
				if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
				break;
			case 1: //Main state
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
				actor->pos.x = check_collision_c(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
				if (actor->pos.x != new_x){
					actor_vel_x[i] = -actor_vel_x[i];
				}
				actor->pos.y = check_collision_c(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
				if (actor->pos.y < new_y){
					actor_vel_y[i] = -20;
					if (actor_vel_x[i] == 0){
						actor_vel_x[i] = -8;
					}
				} else if (actor->pos.y > new_y){
					actor_vel_y[i] = 0;
				}
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
	}			
}