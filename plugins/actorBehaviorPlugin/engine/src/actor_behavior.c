#pragma bank 255

#include <string.h>
#include <stdlib.h>
#include <gbdk/platform.h>
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




UBYTE actor_behavior_ids[MAX_ACTORS];
UBYTE actor_states[MAX_ACTORS];
WORD actor_vel_x[MAX_ACTORS];
WORD actor_vel_y[MAX_ACTORS];
UBYTE actor_counter_a[MAX_ACTORS];
UBYTE actor_counter_b[MAX_ACTORS];
UBYTE actor_linked_actor_idx[MAX_ACTORS];

WORD col_tx;
WORD col_ty;
WORD current_actor_x;
point16_t tmp_point;
const BYTE firebar_incx_lookup[] = { 0, 3, 6, 7, 8, 7, 6, 3, 0, -3, -6, -7, -8, -7, -6, -3 };
const BYTE firebar_incy_lookup[] = { -8, -7, -6, -3, 0, 3, 6, 7, 8, 7, 6, 3, 0, -3, -6, -7 };
const BYTE spring_bb_top_lookup[] = { -8, -4, 0, -4, -8 };

void actor_behavior_init(void) BANKED {
    memset(actor_behavior_ids, 0, sizeof(actor_behavior_ids));
	memset(actor_states, 0, sizeof(actor_states));
	memset(actor_vel_x, 0, sizeof(actor_vel_x));
	memset(actor_vel_y, 0, sizeof(actor_vel_y));
	memset(actor_counter_a, 0, sizeof(actor_counter_a));
	memset(actor_counter_b, 0, sizeof(actor_counter_b));
	memset(actor_linked_actor_idx, 0, sizeof(actor_linked_actor_idx));
}

UWORD check_collision(UWORD start_x, UWORD start_y, bounding_box_t *bounds, col_check_dir_e check_dir) BANKED{    
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

UWORD check_pit(UWORD start_x, UWORD start_y, bounding_box_t *bounds, col_check_dir_e check_dir) BANKED {
     WORD tx, ty;
    switch (check_dir) {
        case CHECK_DIR_LEFT:  // Check left (bottom left)
            tx = (((start_x >> 4) + bounds->left) >> 3);
            ty = (((start_y >> 4) + bounds->bottom) >> 3) + 1;
            if (!(tile_at(tx, ty) & COLLISION_TOP)) {
                return ((tx + 1) << 7) - (bounds->left << 4);
            }
            return start_x;
        case CHECK_DIR_RIGHT:  // Check right (bottom right)
            tx = (((start_x >> 4) + bounds->right) >> 3);
            ty = (((start_y >> 4) + bounds->bottom) >> 3) + 1;
            if (!(tile_at(tx, ty) & COLLISION_TOP)) {
                return (tx << 7) - ((bounds->right + 1) << 4);
            }
            return start_x;       
    }
    return start_x;
}

void apply_gravity(UBYTE actor_idx) BANKED {
	actor_vel_y[actor_idx] += (plat_grav >> 8);
	actor_vel_y[actor_idx] = MIN(actor_vel_y[actor_idx], (plat_max_fall_vel >> 8));
}

void apply_velocity(UBYTE actor_idx, actor_t * actor) BANKED {
	//Apply velocity
	WORD new_y =  actor->pos.y + actor_vel_y[actor_idx];
	WORD new_x =  actor->pos.x + actor_vel_x[actor_idx];
	if (actor->collision_enabled){
		//Tile Collision
		actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
		if (actor->pos.x != new_x){
			actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
		}
		actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
	} else {
		actor->pos.x = new_x;
		actor->pos.y = new_y;
	}
}

void apply_velocity_avoid_fall(UBYTE actor_idx, actor_t * actor) BANKED {
	//Apply velocity
	WORD new_y =  actor->pos.y + actor_vel_y[actor_idx];
	WORD new_x =  actor->pos.x + actor_vel_x[actor_idx];
	if (actor->collision_enabled){
		//Tile Collision
		actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
		if (new_y != actor->pos.y){
			actor->pos.x = check_pit(new_x, actor->pos.y, &actor->bounds, ((actor_vel_x[actor_idx] > 0) ? CHECK_DIR_RIGHT : CHECK_DIR_LEFT));
			if (actor->pos.x != new_x){
				actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
				return;
			}
		}
		actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor_vel_x[actor_idx] > 0) ? CHECK_DIR_RIGHT : CHECK_DIR_LEFT));
		if (actor->pos.x != new_x){
			actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
		}
		
	} else {
		actor->pos.x = new_x;
		actor->pos.y = new_y;
	}
}

void actor_behavior_update(void) BANKED {
	for (UBYTE i = 0; i < MAX_ACTORS; i++){
		actor_t * actor = (actors + i);
		if (!actor->active){
			continue;
		}
		switch(actor_behavior_ids[i]){	
			case 0:
			break;
			case 1: //Goomba
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_counter_a[i] = 0;
						actor_vel_y[i] = 0;
						actor_vel_x[i] = -4;
					}
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					apply_gravity(i);
					apply_velocity(i, actor);
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					break;
				case 2: //Squished state
					if (actor_counter_a[i] == 0){
						actor_reset_anim(actor);
						actor_vel_y[i] = 0;
						actor_vel_x[i] = 0;
						actor->collision_enabled = false;
					}
					actor_counter_a[i]++;
					if (actor_counter_a[i] > 30){
						actor_states[i] = 255; 
					}
					break;
				case 255: //Deactivate
					actor->collision_enabled = true;
					actor_counter_a[i] = 0;
					deactivate_actor(actor);
					break;
			}		
			break;	
			case 2: //Green Koopa
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 						
						actor_vel_y[i] = 0;
						actor_vel_x[i] = -4;
					}
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}					
					apply_gravity(i);
					apply_velocity(i, actor);
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;			
			case 3: //Red Koopa
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 						
						actor_vel_y[i] = 0;
						actor_vel_x[i] = -4;
					}
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}					
					apply_gravity(i);
					apply_velocity_avoid_fall(i, actor);
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;			
			case 4: //Koopa shell
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_counter_a[i] = 0;		
						actor_vel_y[i] = 0;
						actor_vel_x[i] = 0;									
						actor_states[i] = 1;
					}
					break;
				case 1: //tucked state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					apply_gravity(i);
					apply_velocity(i, actor);
					break;
				case 2: //init kicked state
					actor_counter_a[i] = 0;		
					actor_states[i] = 3;
				case 3: //kicked state player iframe
					if (actor_counter_a[i] > 15){
						actor_states[i] = 4; 
					} else {
						actor_counter_a[i]++;
					}
				case 4: //kicked state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					apply_gravity(i);
					//Apply velocity
					WORD new_y =  actor->pos.y + actor_vel_y[i];
					WORD new_x =  actor->pos.x + actor_vel_x[i];
					if (actor->collision_enabled){
						//Tile Collision
						actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
						if (actor->pos.x != new_x){
							actor_vel_x[i] = -actor_vel_x[i];							
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
						actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
					} else {
						actor->pos.x = new_x;
						actor->pos.y = new_y;
					}
					//Actor Collision		
					actor_t * hit_actor = actor_overlapping_bb(&actor->bounds, &actor->pos, actor, FALSE);
					if (hit_actor && hit_actor->script.bank){
						script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 2);
					}
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;
			case 6://Horizontal projectile (Bowser fire, bullet bill)
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						if (actor->script.bank){
							script_execute(actor->script.bank, actor->script.ptr, 0, 1, 8);
						}
					}
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					actor->pos.x =  actor->pos.x + actor_vel_x[i];
					break;
				case 255: //Deactivate
					deactivate_actor(actor);
					break;
			}		
			break;			
			case 7://Pyrahna Plant
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_counter_a[i] = 30;
						actor_vel_y[i] = 0;
					}
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (actor_vel_y[i] > 0){
						actor->pos.y += 16;
						actor_vel_y[i]--;
					}
					else if (((actor->pos.y >> 7) - 2) != PLAYER.pos.y >> 7){ //dont pop out if player is on top
						actor_counter_a[i]--;
						if (actor_counter_a[i] <= 0){
							actor_counter_a[i] = 120;
							actor_vel_y[i] = 16;
							actor_states[i] = 2; 
						}
					}
					break;
				case 2: //Out state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (actor_vel_y[i] > 0){
						actor->pos.y -= 16;
						actor_vel_y[i]--;
					}
					actor_counter_a[i]--;
					if (actor_counter_a[i] <= 0){
						actor_counter_a[i] = 180;
						actor_vel_y[i] = 16;
						actor_states[i] = 1; 
					}
					break;
				case 255: //Deactivate
					deactivate_actor(actor);
					break;
			}		
			break;
			case 8: //Up-down moving actor (Flying Red Koopa, platforms)
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_vel_y[i] = 8;
						actor_counter_a[i] = 0;
					}
					break;
				case 1: //Move up state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (!(game_time & 7)){
						actor_vel_y[i] = MAX(actor_vel_y[i]--, -8);						
						if (actor_counter_a[i] <= 16){
							actor_counter_a[i]++;
						} else {
							actor_states[i] = 2;
						}
					}					
					actor->pos.y = actor->pos.y + actor_vel_y[i];					
					break;
				case 2: //Move down state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (!(game_time & 7)){
						actor_vel_y[i] = MIN(actor_vel_y[i]++, 8);
						if (actor_counter_a[i] > 0){
							actor_counter_a[i]--;
						} else {
							actor_states[i] = 1;
						}	
					}					
					actor->pos.y = actor->pos.y + actor_vel_y[i];									
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;	
			case 9://Fire bar
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}				
					if (!(game_time & 15)){
						actor_counter_a[i] = (actor_counter_a[i] + 1) & 15;
						actor->frame = actor->frame_start + actor_counter_a[i];
					}
					tmp_point.x = (actor->pos.x >> 4) + 4;
					tmp_point.y = (actor->pos.y >> 4) - 28;
					for (UBYTE j = 0; j < 4; j++){		
						if (bb_contains(&PLAYER.bounds, &PLAYER.pos, &tmp_point)){
							script_execute(actor->script.bank, actor->script.ptr, 0, 1, 0);
							break;
						}	
						tmp_point.x += firebar_incx_lookup[actor_counter_a[i]];
						tmp_point.y += firebar_incy_lookup[actor_counter_a[i]];
					}	
					break;
				case 255: //Deactivate
					deactivate_actor(actor);
					break;
			}		
			break;	
			case 10://Bouncing entity
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 10);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
					//Apply velocity
					WORD new_y =  actor->pos.y + actor_vel_y[i];
					WORD new_x =  actor->pos.x + actor_vel_x[i];
					//Tile Collision
					actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
					if (actor->pos.x != new_x){
						actor_vel_x[i] = -actor_vel_x[i];
					}
					actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
					if (actor->pos.y < new_y){
						actor_vel_y[i] = -48;
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
			case 11://Fire ball
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 9);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
					//Apply velocity
					WORD new_y =  actor->pos.y + actor_vel_y[i];
					WORD new_x =  actor->pos.x + actor_vel_x[i];
					//Tile Collision
					actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
					if (actor->pos.x != new_x){
						script_execute(actor->script.bank, actor->script.ptr, 0, 1, 2);
						actor_states[i] = 255;
						break;
					}
					actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
					if (actor->pos.y < new_y){
						actor_vel_y[i] = -40;
					} else if (actor->pos.y > new_y){
						actor_vel_y[i] = 0;
					}
					//Actor Collision					
					actor_t * hit_actor = actor_overlapping_bb(&actor->bounds, &actor->pos, actor, FALSE);
					if (hit_actor && hit_actor->script.bank && actor->collision_group != hit_actor->collision_group){						
						script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 4);
						script_execute(actor->script.bank, actor->script.ptr, 0, 1, 2);
						actor_states[i] = 255;
					}
					break;
				case 255: //Deactivate
					deactivate_actor(actor);
					break;
			}		
			break;		
			case 12: //Growing Beanstalk
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_counter_a[i] = 0;
						actor_states[i] = 1; 
						actor->pos.y = (actor->pos.y >> 7) << 7;
						actor->pos.x = (actor->pos.x >> 7) << 7;
						replace_meta_tile((actor->pos.x >> 7), (actor->pos.y >> 7) - 1, 151);
					}
					break;
				case 1: //Move up state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (actor->pos.y > 384){
						actor->pos.y = actor->pos.y - 8;	
						actor_counter_a[i] = actor_counter_a[i] + 1;
						if (actor_counter_a[i] > 15){
							actor_counter_a[i] = 0;
							replace_meta_tile((actor->pos.x >> 7), (actor->pos.y >> 7) - 1, 151);
							if (tile_at((actor->pos.x >> 7), (actor->pos.y >> 7) - 2) & COLLISION_BOTTOM) {
								actor_states[i] = 255; 
							}
						}
					}	
					else{
						actor_states[i] = 255; 
					}
					break;
				case 255:
					actor_counter_a[i] = 0;
					deactivate_actor(actor);
					break;
			}
			break;
			case 13: //Moving platform (activates on player touch)
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Not moving state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}	
					if (actor_attached && last_actor == actor) {//start moving on player attach						
						actor_states[i] = 2; 
					}					
					break;
				case 2: //moving state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}					
					actor->pos.x = actor->pos.x + actor_vel_x[i];
					actor->pos.y = actor->pos.y + actor_vel_y[i];
					break;
				case 255:
					actor_counter_a[i] = 0;
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
							load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_DEFAULT, PLAYER.animations);
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
			case 15://Knocked enemy
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_vel_y[i] = -40;
						actor_vel_x[i] = 0;
						actor->collision_enabled = false;
					}
					break;
				case 1: //Main state
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD || (actor->pos.y >> 7) > image_tile_height){ 
						actor_states[i] = 255; 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 10);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
					//Apply velocity
					actor->pos.y =  actor->pos.y + actor_vel_y[i];					
					break;
				case 255: //Deactivate
					actor->collision_enabled = true;
					actor_reset_anim(actor);
					deactivate_actor(actor);
					break;
			}		
			break;	
			case 16: //Elevator platform
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //moving state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}	
					if (actor_vel_y[i] < 0 && actor->pos.y < 0) {
						actor->pos.y += (image_tile_height << 7);
						actor_attached = FALSE;
					} else if (actor_vel_y[i] > 0 && (actor->pos.y >> 7) > image_tile_height) {
						actor->pos.y -= (image_tile_height << 7);
						actor_attached = FALSE;
					}						
					actor->pos.y = actor->pos.y + actor_vel_y[i];					
					break;
				case 255:
					actor_counter_a[i] = 0;
					deactivate_actor(actor);
					break;
			}
			break;
			case 17: //Hit block bump
			switch(actor_states[i]){
				case 0:
					//Actor Collision					
					actor_t * hit_actor = actor_overlapping_bb(&actor->bounds, &actor->pos, actor, FALSE);
					if (hit_actor && hit_actor->script.bank){
						script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 2);
					}
					break;
			}
			break;
			case 18: //Sideway moving actor
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_vel_x[i] = 8;
						actor_counter_a[i] = 0;
					}
					break;
				case 1: //Move left state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (!(game_time & 7)){
						actor_vel_x[i] = MAX(actor_vel_x[i]--, -8);						
						if (actor_counter_a[i] <= 16){
							actor_counter_a[i]++;
						} else {
							actor_states[i] = 2;
						}
					}					
					actor->pos.x = actor->pos.x + actor_vel_x[i];					
					break;
				case 2: //Move right state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (!(game_time & 7)){
						actor_vel_x[i] = MIN(actor_vel_x[i]++, 8);
						if (actor_counter_a[i] > 0){
							actor_counter_a[i]--;
						} else {
							actor_states[i] = 1;
						}	
					}					
					actor->pos.x = actor->pos.x + actor_vel_x[i];									
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;	
			case 19: //Falling platform (activates on player touch)
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Not falling state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}	
					if (actor_attached && last_actor == actor) {//start moving on player attach						
						actor_states[i] = 2; 
					}					
					break;
				case 2: //falling state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}					
					actor->pos.y = actor->pos.y + 16;
					if (!actor_attached) {//stop moving on player dettach						
						actor_states[i] = 1; 
					}	
					
					if ((actor->pos.y >> 7) > image_tile_height) {
						actor_states[i] = 255; 
						actor_attached = FALSE;
					}		
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;
			case 20: //Balancing platform (activates on player touch)
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_vel_y[i] = 0;
						actor_counter_a[i] = 0;
					}
					break;
				case 1: //Not moving state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}	
					if (actor_attached && last_actor == actor) {//when player lands on platform						
						actor_states[i] = 3; //move platform down
						actor_states[actor_linked_actor_idx[i]] = 2; //move linked platform up
						actor_vel_y[i] = 0;
						actor_vel_y[actor_linked_actor_idx[i]] = 0;
					}					
					break;
				case 2: //Move up state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (!(game_time & 7)){
						actor_vel_y[i] = MAX(actor_vel_y[i]--, -8);						
						replace_meta_tile((actor->pos.x >> 7), (actor->pos.y >> 7), 0);
					}					
					actor->pos.y = actor->pos.y + actor_vel_y[i];
					if ((actor->pos.y >> 7) < 7){
						actor_states[i] = 4;
						actor_states[actor_linked_actor_idx[i]] = 4;
						actor_vel_y[i] = 0;
					}	
					break;
				case 3: //Move down state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					if (!(game_time & 7)){
						actor_vel_y[i] = MIN(actor_vel_y[i]++, 8);
						replace_meta_tile((actor->pos.x >> 7), (actor->pos.y >> 7) - 1, 19);
					}					
					actor->pos.y = actor->pos.y + actor_vel_y[i];	
					if (!actor_attached) {//when player is not on platform						
						actor_states[i] = 1; //stop moving platform
						actor_states[actor_linked_actor_idx[i]] = 1; //stop moving linked platform
						actor_vel_y[i] = 0;
						actor_vel_y[actor_linked_actor_idx[i]] = 0;
					}					
					break;
				case 4: //Falling apart
					current_actor_x = ((actor->pos.x >> 4) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 10);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
					//Apply velocity
					actor->pos.y =  actor->pos.y + actor_vel_y[i];	
					if ((actor->pos.y >> 7) > image_tile_height) {
						actor_states[i] = 255; 
						actor_attached = FALSE;
					}					
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;
			default:
				actor_behavior_update_b(i, actor);
			break;
		}			
	}
}

void vm_set_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE behavior_id = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_behavior_ids[actor_idx] = behavior_id;
}

void vm_get_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
	script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor_behavior_ids[actor_idx];
}

void vm_set_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE state_id = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_states[actor_idx] = state_id;
}

void vm_get_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
	script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor_states[actor_idx];
}

void vm_set_actor_velocity_x(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    WORD vel_x = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_vel_x[actor_idx] = vel_x;
}

void vm_set_actor_velocity_y(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    WORD vel_y = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_vel_y[actor_idx] = vel_y;
}

void vm_set_actor_linked_actor_idx(SCRIPT_CTX * THIS) OLDCALL BANKED {
	UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE linked_actor_idx = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_linked_actor_idx[actor_idx] = linked_actor_idx;
}