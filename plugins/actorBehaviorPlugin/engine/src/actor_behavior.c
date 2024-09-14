#pragma bank 255

#include <string.h>

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
#include "states/platform.h"
#include "meta_tiles.h"

#define BEHAVIOR_ACTIVATION_THRESHOLD 168
#define BEHAVIOR_DEACTIVATION_THRESHOLD 176

UBYTE actor_behavior_ids[MAX_ACTORS];
UBYTE actor_states[MAX_ACTORS];
WORD actor_vel_x[MAX_ACTORS];
WORD actor_vel_y[MAX_ACTORS];
UBYTE actor_counter_a[MAX_ACTORS];

upoint16_t tmp_point;
const BYTE firebar_incx_lookup[] = { 0, 3, 6, 7, 8, 7, 6, 3, 0, -3, -6, -7, -8, -7, -6, -3 };
const BYTE firebar_incy_lookup[] = { -8, -7, -6, -3, 0, 3, 6, 7, 8, 7, 6, 3, 0, -3, -6, -7 };

void actor_behavior_init(void) BANKED {
    memset(actor_behavior_ids, 0, sizeof(actor_behavior_ids));
	memset(actor_states, 0, sizeof(actor_states));
	memset(actor_vel_x, 0, sizeof(actor_vel_x));
	memset(actor_vel_y, 0, sizeof(actor_vel_y));
	memset(actor_counter_a, 0, sizeof(actor_counter_a));
}

UWORD check_collision(UWORD start_x, UWORD start_y, bounding_box_t *bounds, col_check_dir_e check_dir) BANKED{
    WORD tx, ty;
    switch (check_dir) {
        case CHECK_DIR_LEFT:  // Check left (bottom left)
            tx = (((start_x >> 4) + bounds->left) >> 3);
            ty = (((start_y >> 4) + bounds->bottom) >> 3);
            if (tile_at(tx, ty) & COLLISION_RIGHT) {
                return ((tx + 1) << 7) - (bounds->left << 4);
            }
            return start_x;
        case CHECK_DIR_RIGHT:  // Check right (bottom right)
            tx = (((start_x >> 4) + bounds->right) >> 3);
            ty = (((start_y >> 4) + bounds->bottom) >> 3);
            if (tile_at(tx, ty) & COLLISION_LEFT) {
                return (tx << 7) - ((bounds->right + 1) << 4);
            }
            return start_x;
        case CHECK_DIR_UP:  // Check up (middle up)
            ty = (((start_y >> 4) + bounds->top) >> 3);
            tx = (((start_x >> 4) + ((bounds->left + bounds->right) >> 1)) >> 3);
            if (tile_at(tx, ty) & COLLISION_BOTTOM) {
                return ((ty + 1) << 7) - ((bounds->top) << 4);
            }
            return start_y;
        case CHECK_DIR_DOWN:  // Check down (right bottom and left bottom)
            ty = (((start_y >> 4) + bounds->bottom) >> 3);
            tx = (((start_x >> 4) + bounds->left) >> 3);
            if (tile_at(tx, ty) & COLLISION_TOP) {
                return ((ty) << 7) - ((bounds->bottom + 1) << 4);
            }			
			tx = (((start_x >> 4) + bounds->right) >> 3);
			if (tile_at(tx, ty) & COLLISION_TOP) {
                return ((ty) << 7) - ((bounds->bottom + 1) << 4);
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

void inline apply_gravity(UBYTE actor_idx) {
	actor_vel_y[actor_idx] += (plat_grav >> 8);
	actor_vel_y[actor_idx] = MIN(actor_vel_y[actor_idx], (plat_max_fall_vel >> 8));
}

void inline apply_velocity(UBYTE actor_idx, actor_t * actor) {
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

void inline apply_velocity_avoid_fall(UBYTE actor_idx, actor_t * actor) {
	//Apply velocity
	WORD new_y =  actor->pos.y + actor_vel_y[actor_idx];
	WORD new_x =  actor->pos.x + actor_vel_x[actor_idx];
	if (actor->collision_enabled){
		//Tile Collision
		new_x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
		actor->pos.x = check_pit(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
		if (actor->pos.x != new_x){
			actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
		}
		actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
	} else {
		actor->pos.x = new_x;
		actor->pos.y = new_y;
	}
}

void actor_behavior_update(void) BANKED {
	for (UBYTE i = 1; i < MAX_ACTORS; i++){
		actor_t * actor = (actors + i);
		if (!actor->active){
			continue;
		}
		switch(actor_behavior_ids[i]){			
			case 1: //Goomba
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
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
				case 255: //Deactivate
					deactivate_actor(actor);
					break;
			}		
			break;	
			case 2: //Koopa
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
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
			case 3://Bowser
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					apply_gravity(i);
					apply_velocity(i, actor);
					//Animation
					if (PLAYER.pos.x < actor->pos.x) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					}
					break;
				case 2: //Jump state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 10);
					apply_velocity(i, actor);
					//Animation
					if (PLAYER.pos.x < actor->pos.x) {
						actor_set_anim(actor, ANIM_JUMP_LEFT);
					} else {
						actor_set_anim(actor, ANIM_JUMP_RIGHT);
					}
					if (actor_vel_y[i] > 0){
						actor_states[i] = 1;
					}
					break;
				case 255: //Deactivate
					deactivate_actor(actor);
					break;
			}		
			break;	
			case 4://Bowser fire
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
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
			case 5://Pyrahna Plant
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_counter_a[i] = 180;
					}
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
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
			case 6: //Red Koopa
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
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
			case 7: //Flying Red Koopa
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Move up state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					
					if (actor_counter_a[i] < 120){
						actor_counter_a[i]++;
					} else {
						actor_states[i] = 2;
						actor_vel_y[i] = -actor_vel_y[i];
						actor_vel_x[i] = -actor_vel_x[i];
					}
					actor->pos.y = actor->pos.y + actor_vel_y[i];		
					actor->pos.x = actor->pos.x + actor_vel_x[i];						
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					break;
				case 2: //Move down state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					
					if (actor_counter_a[i] > 0){
						actor_counter_a[i]--;
					} else {
						actor_states[i] = 1;
						actor_vel_y[i] = -actor_vel_y[i];
						actor_vel_x[i] = -actor_vel_x[i];
					}
					actor->pos.y = actor->pos.y + actor_vel_y[i];
					actor->pos.x = actor->pos.x + actor_vel_x[i];	
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
			case 8://Fire bar
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}					
					if (!(game_time & 7)){
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
			case 9://Bouncing entity
			switch(actor_states[i]){
				case 0: //Init
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
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
			case 10: //Koopa shell
			switch(actor_states[i]){
				case 0:
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					if ((((actor->pos.x >> 4) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					apply_gravity(i);
					apply_velocity(i, actor);
					//Actor Collision					
					actor_t * hit_actor = actor_overlapping_bb(&actor->bounds, &actor->pos, actor, FALSE);
					if (hit_actor && hit_actor->script.bank){
						script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, (UWORD)(actor->collision_group));
					}
					break;
				case 255:
					deactivate_actor(actor);
					break;
			}
			break;		
		}			
	}
}

void vm_set_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE behavior_id = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_behavior_ids[actor_idx] = behavior_id;
}

void vm_set_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE state_id = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_states[actor_idx] = state_id;
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
/*
void vm_set_actor_spritesheet_ex(SCRIPT_CTX * THIS) OLDCALL BANKED {
	UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE base_tile = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
	UBYTE spritesheet_bank = *(uint8_t *)VM_REF_TO_PTR(FN_ARG2);
	const spritesheet_t * spritesheet = *(const spritesheet_t * *)VM_REF_TO_PTR(FN_ARG3);	
    actor_t * actor = (actors + actor_idx);	    
	actor->base_tile = base_tile;
    actor->sprite.bank = spritesheet_bank;
    actor->sprite.ptr = (void *)spritesheet;
    load_animations(spritesheet, spritesheet_bank, ANIM_SET_DEFAULT, actor->animations);
    load_bounds(spritesheet, spritesheet_bank, &actor->bounds);
    actor_reset_anim(actor);
}

void vm_load_spritesheet_ex(SCRIPT_CTX * THIS) OLDCALL BANKED {
	UBYTE base_tile = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
	UBYTE spritesheet_bank = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
	const spritesheet_t * spritesheet = *(const spritesheet_t * *)VM_REF_TO_PTR(FN_ARG2);
	load_sprite(base_tile, spritesheet, spritesheet_bank);
}



//Actor pooling
UBYTE poolable_actors[MAX_ACTORS];

void vm_init_actor_pool(SCRIPT_CTX * THIS) OLDCALL BANKED {
	UBYTE pool_start_idx = MAX(*(uint8_t *)VM_REF_TO_PTR(FN_ARG0), 1);
	UBYTE pool_end_idx = MIN(*(uint8_t *)VM_REF_TO_PTR(FN_ARG1), MAX_ACTORS);
	//Init actor pool
	memset(poolable_actors, 0, sizeof(poolable_actors));
	for (UBYTE i = pool_start_idx; i < pool_end_idx; i++){
		poolable_actors[i] = 1;
	}
}

void vm_instanciate_actor(SCRIPT_CTX * THIS) OLDCALL BANKED {
	UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
	WORD actor_pos_x = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
	WORD actor_pos_y = *(int16_t *)VM_REF_TO_PTR(FN_ARG2);
	UBYTE actor_bank = *(uint8_t *)VM_REF_TO_PTR(FN_ARG3);
	const void* actor_ptr = *(const void* *)VM_REF_TO_PTR(FN_ARG4);
	for (UBYTE i = 1; i < MAX_ACTORS; i++){
		if (poolable_actors[i] == 1){
			actor_t * actor = actors + i;
			if (actor->active){
				DL_REMOVE_ITEM(actors_active_head, actor);
			} else {
				DL_REMOVE_ITEM(actors_inactive_head, actor);
			}
			MemcpyBanked(actor, actor_ptr + actor_idx, sizeof(actor_t), actor_bank);
			actor->active = FALSE;
			actor->pos.x = actor_pos_x;
			actor->pos.y = actor_pos_y;
            DL_PUSH_HEAD(actors_inactive_head, actor);
			poolable_actors[i] = 0;
			break;
		}
	}
	
}
*/