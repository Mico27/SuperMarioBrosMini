/*
Future notes on things to do:
- Limit air dashes before touching the ground
- Add an option for wall jump that only allows alternating walls.
- Currently, dashing through walls checks the potential end point, and if it isn't clear then it continues with the normal dash routine. 
    The result is that there could be a valid landing point across a wall, but the player is just a little too close for it to register. 
    I could create a 'look-back' loop that runs through the intervening tiles until it finds an empty landing spot.
- The bounce event is a funny one, because it can have the player going up without being in the jump state. I should perhaps add some error catching stuff for such situations
- Can I have a wall_jump init ahead of the normal jump init? If it's just checking a few more boxes....
- Improve ladder situation: jump from ladder option, bug with hitting the bottom of ladders, other stuff?

- Add check for camera bounds on Dash Init
- Solid actors have a fault: they only check collisions once on enter. This is especially problematic because of how GBStudio does invincibility frames,
  because it means the player can attach to a platform without causing the hit trigger to run. 
     - 2 potential solutions: 
     - give the player a minimum velocity every frame, forcing a re-collision. This might break the moving platform code though
     - manually re-trigger the collision call if the actor is attached.

TARGETS for Optimization
- State script assignment could be 100% be re-written to avoid all those assignments and directly use the pointers. I am not canny enough to do that.
- I should be able to combine solid actors and platform actors into a single check...
- It's inellegant that the dash check requires me to check again later if it succeeded or not. Can I reorganize this somehow?
- I think I can probably combine actor_attached and last_actor
- I need to refactor the downwards collision for Y, it's a bit of a mess at this point. I just can't wrap my head around it atm
- Wall Slide could be optimized to skill the acceleration bit, as the only thing that matters is tapping away

THINGS TO WATCH
- Does every state (that needs to) end up resetting DeltaX?

NOTES on GBStudio Quirks
- 256 velocities per position, 16 'positions' per pixel, 8 pixels per tile
- Player bounds: for an ordinary 16x16 sprite, bounds.left starts at 0 and bounds.right starts at 16. If it's smaller, bounds.left is POSITIVE
    For bounds.top, however, Y starts counting from the middle of a sprite. bounds.top is negative and bounds.bottom is positive
- CameraX is in the middle of the screen, not left corner

GENERAL STRUCTURE OF THIS FILE
The old format was well structured as a state-machine, isolating all the components into states. Unfortunately, it seems like the overhead of calling 
collision functions on the GameBoy makes this model unperformant. However, I'm also limited to the total amount of code that can be placed in a single bank. 
I cannot get rid of the functions and move the code into the file itself. New structure is a compromise that uses goto commands to skip some sections that are
shared by most of the states. 
    
INIT()
    Tweak a few fields so they don't overflow variables
    Normalize some fields so that they are applied over multiple frames
    Initialize variables
UPDATE()
    A. Input Checks (Double-tap Dash, Drop-Through)    I'm considering moving the drop-through check into a state
    B. STATE MACHINE 1 SWITCH: Falling, Ground, Jumping, Dashing, Climbing a Ladder, Wall Sliding
        State Initialization
        Calculate Change in Vertical Movement
        Some calculate horziontal movement
        Some calculate collisions
    C. Shared Collision
        Acceleration Code   
        Basic X Collision           gotoXCol
        Basic Y Collision           gotoYCol
        Actor Collision Check       gotoActorCol
    D. STATE MACHINE 2 SWITCH:      gotoSwitch2
        Animation
        State Change Logic
        Some Counters
    E. Trigger Check                gotoTriggerCol
    G. Tic Counters                 gotoCounters


BUGS:
 - When the player is on a moving platform and is hit by another one, they get caught mid-way on the next one.
*/
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
#include "meta_tiles.h"
#include "data/game_globals.h"
#include "actor_behavior.h"

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

#define MAX_METATILE_CHANGE_CHECKS 4

script_state_t state_events[24];

script_state_t specific_events[6];

UBYTE grounded;

//DEFAULT ENGINE VARIABLES
WORD plat_min_vel;
WORD plat_walk_vel;
WORD plat_run_vel;
WORD plat_climb_vel;
WORD plat_walk_acc;
WORD plat_run_acc;
WORD plat_dec;
WORD plat_jump_vel;
WORD plat_grav;
WORD plat_hold_grav;
WORD plat_max_fall_vel;

//PLATFORMER PLUS ENGINE VARIABLES
//All engine fields are prefixed with plat_
BYTE plat_camera_deadzone_x; // Camera deadzone
UBYTE plat_camera_block;    //Limit the player's movement to the camera's edges
UBYTE plat_drop_through;    //Drop-through control
UBYTE plat_mp_group;        //Collision group for platform actors
UBYTE plat_solid_group;     //Collision group for solid actors
WORD plat_jump_min;         //Jump amount applied on the first frame of jumping
UBYTE plat_hold_jump_max;   //Maximum number for frames for continuous input
WORD plat_jump_reduction;   //Reduce height each double jump
UBYTE plat_coyote_max;      //Coyote Time maximum frames
UBYTE plat_buffer_max;      //Jump Buffer maximum frames
UBYTE plat_float_input;     //Input type for float (hold up or hold jump)
WORD plat_float_grav;       //Speed of fall descent while floating
UBYTE plat_turn_control;    //Controls the amount of slippage when the player turns while running.
WORD plat_air_dec;          // air deceleration rate
WORD plat_turn_acc;         //Speed with which a character turns
UBYTE plat_run_boost;        //Additional jump height based on player horizontal speed
BYTE run_stage;             //Tracks the stage of running based on the run type

UBYTE nocollide;            //Turns off vertical collisions, currently only for dropping through platforms
WORD deltaX;
WORD deltaY;

//COUNTER variables
UBYTE ct_val;               //Coyote Time Variable
UBYTE jb_val;               //Jump Buffer Variable
UBYTE hold_jump_val;        //Jump input hold variable

//WALL variables 
BYTE last_wall;             //tracks the last wall the player touched
BYTE col;

//COLLISION VARS
actor_t *last_actor;        //The last actor the player hit, and that they were attached to
UBYTE actor_attached;       //Keeps track of whether the player is currently on an actor and inheriting its movement
WORD mp_last_x;             //Keeps track of the pos.x of the attached actor from the previous frame
WORD mp_last_y;             //Keeps track of the pos.y of the attached actor from the previous frame


//JUMPING VARIABLES
WORD jump_reduction_val;    //Holds a temporary jump velocity reduction
WORD jump_per_frame;        //Holds a jump amount that has been normalized over the number of jump frames
WORD jump_reduction;        //Holds the reduction amount that has been normalized over the number of jump frames
WORD boost_val;
UBYTE enemy_bounce;

//WALKING AND RUNNING VARIABLES
WORD pl_vel_x;              //Tracks the player's x-velocity between frames
WORD pl_vel_y;              //Tracks the player's y-velocity between frames

//VARIABLES FOR CAMERAS
WORD *edge_left;
WORD *edge_right;
WORD mod_image_right;
WORD mod_image_left;

//TEMP VARIABLES
WORD temp_counter;


UBYTE previous_top_tile;
UBYTE previous_left_tile;
UBYTE previous_bottom_tile;
UBYTE previous_right_tile;

UBYTE current_left_tile; 
UBYTE current_top_tile;	
UBYTE current_right_tile; 
UBYTE current_bottom_tile;
	
UBYTE actors_collisionx_cache[4];
UBYTE actors_collisiony_cache[4];

UBYTE update_ui_bank;
UBYTE* update_ui_ptr;

UBYTE hit_block_bank;
UBYTE* hit_block_ptr;

UBYTE current_vine_tile_x;

void platform_init(void) BANKED {
    //Initialize Camera
    camera_offset_x = 0;
    camera_offset_y = 0;
    camera_deadzone_x = plat_camera_deadzone_x;
    camera_deadzone_y = PLATFORM_CAMERA_DEADZONE_Y;
    if ((camera_settings & CAMERA_LOCK_X_FLAG)){
        camera_x = (PLAYER.pos.x >> 4) + 8;
    } else{
        camera_x = 0;
    }
    if ((camera_settings & CAMERA_LOCK_Y_FLAG)){
        camera_y = (PLAYER.pos.y >> 4) + 8;
    } else{
        camera_y = 0;
    }

    //Initialize Camera Bounds
    mod_image_right = image_width - SCREEN_WIDTH;
    mod_image_left = 0;
    if (plat_camera_block & 1){
        edge_left = &scroll_x;
    }
    else{
        edge_left = &mod_image_left;
    }

    if (plat_camera_block & 2){
        edge_right = &scroll_x;
    }
    else{
        edge_right = &image_width;
    }

    
    //Make sure jumping doesn't overflow variables
    //First, check for jumping based on Frames and Initial Jump Min
    while (32000 - (plat_jump_vel/MIN(15,plat_hold_jump_max)) - plat_jump_min < 0){
        plat_hold_jump_max += 1;
    }

    //This ensures that, by itself, the plat run boost active on any single frame cannot overflow a WORD.
    //It is complemented by another check in the jump itself that works with the actual velocity. 
    if (plat_run_boost != 0){
        while((32000/plat_run_boost) < ((plat_run_vel>>8)/plat_hold_jump_max)){
            plat_run_boost--;
        }
    }

    //Normalize variables by number of frames
    jump_per_frame = plat_jump_vel / MIN(15, plat_hold_jump_max);   //jump force applied per frame in the JUMP_STATE
    jump_reduction = plat_jump_reduction / plat_hold_jump_max;      //Amount to reduce subequent jumps per frame in JUMP_STATE
    boost_val = plat_run_boost / plat_hold_jump_max;                  //Vertical boost from horizontal speed per frame in JUMP STATE

    //Initialize State
    plat_state = GROUND_INIT;
    que_state = GROUND_INIT;
    actor_attached = FALSE;
    nocollide = 0;
    if (PLAYER.dir == DIR_UP || PLAYER.dir == DIR_DOWN || PLAYER.dir == DIR_NONE) {
        PLAYER.dir = DIR_RIGHT;
    }

    //Initialize other vars
    game_time = 0;
    pl_vel_x = 0;
    pl_vel_y = 4000;                //Magic number for preventing a small glitch when loading into a scene
    hold_jump_val = plat_hold_jump_max;
    deltaX = 0;
    deltaY = 0;
	actor_behavior_init();
}

void platform_update(void) BANKED {
    //The switch here is to disagreggate the state code, because it had collectively grown too large to stay in a single bank.
	actor_behavior_update();
    plat_state = que_state;
	script_memory[VAR_FRAMECOINS] = 0;
    switch(plat_state){
        case FALL_INIT:
            que_state = FALL_STATE;
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_DEFAULT, PLAYER.animations);
        case FALL_STATE:
            fall_state();
            break;
        case GROUND_INIT:
            que_state = GROUND_STATE;
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_DEFAULT, PLAYER.animations);
            //pl_vel_y = 256;
            ct_val = plat_coyote_max; 
            jump_reduction_val = 0;
			enemy_bounce = 0;
        case GROUND_STATE:
            ground_state();
            break;
		case CROUCH_INIT:
			que_state = CROUCH_STATE;
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_CROUCH, PLAYER.animations);
			PLAYER.bounds.top = 1;
			//pl_vel_y = 256;
		case CROUCH_STATE:
			crouch_state();
			break;
		case SKID_INIT:
			que_state = SKID_STATE;
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_SKID, PLAYER.animations);
		case SKID_STATE:
			ground_state();
			break;
        case JUMP_INIT:
            que_state = JUMP_STATE;
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_DEFAULT, PLAYER.animations);
            hold_jump_val = plat_hold_jump_max; 
            actor_attached = FALSE;
            pl_vel_y = -plat_jump_min;
            jb_val = 0;
            ct_val = 0;
        case JUMP_STATE:
            jump_state();
            break;
        case CLIMB_INIT:
			que_state = CLIMB_STATE;
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_CLIMB, PLAYER.animations);
			pl_vel_y = 0;
			pl_vel_x = 0;	
		case CLIMB_STATE:
			climb_state();
			break;	
        case BLANK_INIT:
            que_state = BLANK_STATE;
            pl_vel_x = 0;
            pl_vel_y = 0;
        case BLANK_STATE:
            //State-Based Events
			if(state_events[plat_state].script_addr != 0){
				script_execute(state_events[plat_state].script_bank, state_events[plat_state].script_addr, 0, 0);
			}
            break;
		case SWIM_INIT:
			que_state = SWIM_STATE;
			load_animations(PLAYER.sprite.ptr, PLAYER.sprite.bank, ANIM_STATE_SWIM, PLAYER.animations);
			hold_jump_val = plat_hold_jump_max; 
			actor_attached = FALSE;
			pl_vel_y = -plat_jump_min;
		case SWIM_STATE:
			swim_state();
			break;
    }
	if (script_memory[VAR_PLAYER_IFRAMES] > 0){
		script_memory[VAR_PLAYER_IFRAMES]--;
	}
	check_player_metatiles_entered();
	if(script_memory[VAR_FRAMECOINS] && specific_events[COIN_COLLECTED_EVENT].script_addr != 0){
        script_execute(specific_events[COIN_COLLECTED_EVENT].script_bank, specific_events[COIN_COLLECTED_EVENT].script_addr, 0, 0);
    }
}


inline void on_metatile_enter(UBYTE tile_x, UBYTE tile_y) {
	UBYTE tile_id = sram_map_data[VRAM_OFFSET(tile_x, tile_y)];
	switch(tile_id){
		case 4://coin
			replace_meta_tile(tile_x, tile_y, 0);
			script_memory[VAR_FRAMECOINS]++;
			break;
		case 1://underwater coin
			replace_meta_tile(tile_x, tile_y, 2);
			script_memory[VAR_FRAMECOINS]++;
			break;
	}
}

void check_player_metatiles_entered(void) BANKED {
	
    current_left_tile = ((PLAYER.pos.x >> 4) + PLAYER.bounds.left) >> 3; 
    current_top_tile = ((PLAYER.pos.y >> 4) + PLAYER.bounds.top) >> 3;	
	current_right_tile = ((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3; 
    current_bottom_tile = ((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3;	
		
	UBYTE tmp_tile = 0;
	UBYTE left_side_checked = 0;
	UBYTE right_side_checked = 0;
	UBYTE max_counter = 0;
	
	//check left
	if (current_left_tile < previous_left_tile){
		tmp_tile = current_top_tile;
		while (max_counter < MAX_METATILE_CHANGE_CHECKS && tmp_tile <= current_bottom_tile){
			on_metatile_enter(current_left_tile, tmp_tile);			
			tmp_tile++;
			max_counter++;
		}
		max_counter = 0;
		left_side_checked = 1;
	}
	//check right	
	if (current_left_tile != current_right_tile && (current_right_tile > previous_right_tile)){
		tmp_tile = current_top_tile;
		while (max_counter < MAX_METATILE_CHANGE_CHECKS && tmp_tile <= current_bottom_tile){
			on_metatile_enter(current_right_tile, tmp_tile);
			tmp_tile++;
			max_counter++;
		}
		max_counter = 0;
		right_side_checked = 1;
	}
	
	//Check top
	if (current_top_tile < previous_top_tile){
		tmp_tile = current_left_tile + left_side_checked;
		while (max_counter < MAX_METATILE_CHANGE_CHECKS && tmp_tile <= (current_right_tile - right_side_checked)){
			on_metatile_enter(tmp_tile, current_top_tile);			
			tmp_tile++;
			max_counter++;
		}
		max_counter = 0;
	}	
	
	//Check bottom
	if (current_top_tile != current_bottom_tile && (current_bottom_tile > previous_bottom_tile)){
		tmp_tile = current_left_tile + left_side_checked;
		while (max_counter < MAX_METATILE_CHANGE_CHECKS && tmp_tile <= (current_right_tile - right_side_checked)){
			on_metatile_enter(tmp_tile, current_bottom_tile);			
			tmp_tile++;
			max_counter++;
		}
		max_counter = 0;
	}
	
	previous_left_tile = current_left_tile;
	previous_top_tile = current_top_tile;
	previous_right_tile = current_right_tile;
	previous_bottom_tile = current_bottom_tile;
	
}

void on_player_metatile_collision(UBYTE tile_x, UBYTE tile_y, UBYTE direction) BANKED{	
	
	if (actors_collisionx_cache[direction] != tile_x || actors_collisiony_cache[direction] != tile_y) {
		actors_collisionx_cache[direction] = tile_x;
		actors_collisiony_cache[direction] = tile_y;
		switch(direction){
			case DIR_UP:
				UBYTE tile_id = sram_map_data[VRAM_OFFSET(tile_x, tile_y)];
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
					case 160://invis coin block
					case 161://invis powerup block
					case 162://invis star block
					case 163://invis 1up block	
					case 169://beanstalk brick
						if(specific_events[HIT_BLOCK_EVENT].script_addr != 0){
							script_memory[VAR_HITBLOCKID] = tile_id;
							script_memory[VAR_HITBLOCKX] = tile_x;
							script_memory[VAR_HITBLOCKY] = tile_y;
							script_execute(specific_events[HIT_BLOCK_EVENT].script_bank, specific_events[HIT_BLOCK_EVENT].script_addr, 0, 0);
						}
						break;
				}
				break;
		}
	}
}

void reset_collision_cache(UBYTE direction) BANKED {	
	actors_collisionx_cache[direction] = 0;
	actors_collisiony_cache[direction] = 0;	
}


void fall_state(void) BANKED {
    //INITIALIZE VARS
    WORD temp_y = 0;
    UBYTE p_half_width = (PLAYER.bounds.right - PLAYER.bounds.left) >> 1;
    UBYTE tile_x_mid = ((PLAYER.pos.x >> 4) + PLAYER.bounds.left + p_half_width) >> 3; 
    UBYTE tile_y = ((PLAYER.pos.y >> 4) + PLAYER.bounds.top + 1) >> 3;
    col = 0;
    
    //A. INPUT CHECK=================================================================================================
       
    UBYTE drop_press =  FALSE;
    switch(plat_drop_through){
        case 1:
        if(INPUT_DOWN){
            drop_press = TRUE;
        }
        break;
        case 2:
        if (INPUT_PRESSED(INPUT_DOWN)){
            drop_press = TRUE;
        }
        break;
        case 3:
        if (INPUT_DOWN && INPUT_PLATFORM_JUMP){
            drop_press = TRUE;
        }
        break;
        case 4:
        if ((INPUT_PRESSED(INPUT_DOWN) && INPUT_PLATFORM_JUMP) || (INPUT_DOWN && INPUT_PRESSED(INPUT_PLATFORM_JUMP))){
            drop_press = TRUE;
        }
        break;
    }


    //B. STATE SPECIFIC LOGIC
    //Vertical Movement--------------------------------------------------------------------------------------------
    //FLOAT INPUT
    if (((plat_float_input == 1 && INPUT_PLATFORM_JUMP) || (plat_float_input == 2 && INPUT_UP)) && pl_vel_y >= 0){
        pl_vel_y = plat_float_grav;
    } else if (nocollide != 0){
        //magic number, rough minimum for actually having the player descend through a platform
        pl_vel_y = 7000; 
    } else if (INPUT_PLATFORM_JUMP && pl_vel_y < 0) {
        //Gravity while holding jump
        pl_vel_y += plat_hold_grav;
        pl_vel_y = MIN(pl_vel_y,plat_max_fall_vel);
    } else {
        //Normal gravity
        pl_vel_y += plat_grav;
        pl_vel_y = MIN(pl_vel_y,plat_max_fall_vel);
    }

    //Collision ---------------------------------------------------------------------------------------------------
    //Vertical Collision Checks
    deltaY += pl_vel_y >> 8;
    temp_y = PLAYER.pos.y;    

    //Horizontal Movement----------------------------------------------------------------------------------------

    //C. ACCELERATION
    if (INPUT_LEFT || INPUT_RIGHT){
        BYTE dir = 1;
        if (INPUT_LEFT){
            dir = -1;
            pl_vel_x = -pl_vel_x;
        }

        if (INPUT_PLATFORM_RUN){
            //Type 1: Smooth Acceleration as the Default in GBStudio
            pl_vel_x = CLAMP(pl_vel_x + plat_run_acc, plat_min_vel, plat_run_vel);
            pl_vel_x *= dir;
            deltaX += pl_vel_x >> 8;
            run_stage = 1;
        } else {
            //Ordinay Walk
            if(pl_vel_x < 0 && plat_turn_acc != 0){
                pl_vel_x += plat_turn_acc;
                run_stage = -1;
            } else {
                run_stage = 0;
                pl_vel_x += plat_walk_acc;
                pl_vel_x = CLAMP(pl_vel_x, plat_min_vel, plat_walk_vel); 
            }
            pl_vel_x *= dir;
            deltaX += pl_vel_x >> 8;

        }
    } else{
        //DECELERATION
        if (pl_vel_x < 0) {
            pl_vel_x += plat_air_dec;
            if (pl_vel_x > 0) {
                pl_vel_x = 0;
            }
        } else if (pl_vel_x > 0) {
            pl_vel_x -= plat_air_dec; 
            if (pl_vel_x < 0) {
                pl_vel_x = 0;
            }
        }
        run_stage = 0;
        deltaX += pl_vel_x >> 8;
    }

    //D. COLLISIONS
    //X-COLLISION
    gotoXCol:
    {
        deltaX = CLAMP(deltaX, -127, 127);
        UBYTE tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
        UBYTE tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;       
        UWORD new_x = PLAYER.pos.x + deltaX;
        
        UBYTE tile_x = 0;
        UBYTE col_mid = 0;
        
        //Edge Locking
        //If the player is past the right edge (camera or screen)
        if (new_x > (*edge_right + SCREEN_WIDTH - 16) <<4){
            //If the player is trying to go FURTHER right
            if (new_x > PLAYER.pos.x){
                new_x = PLAYER.pos.x;
                pl_vel_x = 0;
            } else {
            //If the player is already off the screen, push them back
                new_x = PLAYER.pos.x - MIN(PLAYER.pos.x - ((*edge_right + SCREEN_WIDTH - 16)<<4), 16);
            }
        //Same but for left side. This side needs a 1 tile (8px) buffer so it doesn't overflow the variable.
        } else if (new_x < *edge_left << 4){
            if (deltaX < 0){
                new_x = PLAYER.pos.x;
                pl_vel_x = 0;
            } else {
                new_x = PLAYER.pos.x + MIN(((*edge_left+8)<<4)-PLAYER.pos.x, 16);
            }
        }

        //Step-Check for collisions one tile left or right for each avatar height tile
        if (new_x > PLAYER.pos.x) {
            tile_x = ((new_x >> 4) + PLAYER.bounds.right) >> 3;
			switch(sram_map_data[VRAM_OFFSET(tile_x, tile_end - 1)]){
				case 151: //beanstalk tile
					que_state = CLIMB_INIT; 
					current_vine_tile_x = tile_x;
					actor_set_dir(&PLAYER, DIR_LEFT, FALSE);
					break;
			}
            
            while (tile_start < tile_end) {
               
                col = tile_at(tile_x, tile_start);
                if (col & COLLISION_LEFT) {				
					new_x = (((tile_x << 3) - PLAYER.bounds.right) << 4) - 1;
					pl_vel_x = 0;
					col = 1;
					last_wall = 1;	
					on_player_metatile_collision(tile_x, tile_start, DIR_RIGHT); 					
					break;
                } else {
					reset_collision_cache(DIR_RIGHT);
				}
                tile_start++;
            }
        } else if (new_x < PLAYER.pos.x) {
            tile_x = ((new_x >> 4) + PLAYER.bounds.left) >> 3;
            switch(sram_map_data[VRAM_OFFSET(tile_x, tile_end - 1)]){
				case 151: //beanstalk tile
					que_state = CLIMB_INIT; 
					current_vine_tile_x = tile_x;
					actor_set_dir(&PLAYER, DIR_RIGHT, FALSE);
					break;
			}
            while (tile_start < tile_end) {
                col = tile_at(tile_x, tile_start);
                if (col & COLLISION_RIGHT) {					
                    
                    new_x = ((((tile_x + 1) << 3) - PLAYER.bounds.left) << 4) + 1;
                    pl_vel_x = 0;
                    col = -1;
                    last_wall = -1;		
					on_player_metatile_collision(tile_x, tile_start, DIR_LEFT); 					
                    break;
                } else {
					reset_collision_cache(DIR_LEFT);
				}
                tile_start++;
            }
        }
        PLAYER.pos.x = new_x;
    }

    //Y-COLLISION
    {
        deltaY = CLAMP(deltaY, -127, 127);

        //New Y Slopes 1
        UBYTE prev_grounded = grounded;
        UWORD old_y = PLAYER.pos.y;
        grounded = FALSE;
        //End New Y Slopes 1

        UBYTE tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
        UBYTE tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
        if (deltaY > 0) {
            //Moving Downward
            WORD new_y = PLAYER.pos.y + deltaY;
            tile_y = ((new_y >> 4) + PLAYER.bounds.bottom) >> 3;

            if (nocollide == 0){
                //Check collisions from left to right with the bottom of the player
                while (tile_start < tile_end) {
                    if (tile_at(tile_start, tile_y) & COLLISION_TOP) {						
                        //Drop-Through Floor Check 
                        if (drop_press){
                            //If it's a regular tile, do not drop through
                            while (tile_start < tile_end) {
                                if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM){
                                    //Escape two levels of looping. 
                                    goto land;
                                }
                            tile_start++;
                            }
                            nocollide = 5; //Magic Number, how many frames to steal vertical control
                            pl_vel_y += plat_grav;
                            break; 
                        }
                        //Land on Floor
                        land:
                        new_y = ((((tile_y) << 3) - PLAYER.bounds.bottom) << 4) - 1;
                        actor_attached = FALSE; //Detach when MP moves through a solid tile.
                        //The distinction here is used so that we can check the velocity when the player hits the ground.
                        if(plat_state == GROUND_STATE){
                            que_state = GROUND_STATE; 
                            pl_vel_y = 0;
                        } else if(plat_state == GROUND_INIT){
                            que_state = GROUND_STATE;
                        } else {que_state = GROUND_INIT;}		
						on_player_metatile_collision(tile_start, tile_y, DIR_DOWN);						
                        break;
                    } else {
						reset_collision_cache(DIR_DOWN);
					}
                    tile_start++;
                }
            }
            PLAYER.pos.y = new_y;

        } else if (deltaY < 0) {
            //Moving Upward
            WORD new_y = PLAYER.pos.y + deltaY;
            UBYTE tile_y = (((new_y >> 4) + PLAYER.bounds.top) >> 3);
            while (tile_start < tile_end) {
                if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM) {					
                    new_y = ((((UBYTE)(tile_y + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
                    pl_vel_y = 0;
                    //MP Test: Attempting stuff to stop the player from continuing upward
                    if(actor_attached){
                        temp_y = last_actor->pos.y;
                        if (last_actor->bounds.top > 0){
                            temp_y += last_actor->bounds.top + last_actor->bounds.bottom << 5;
                        }
                        new_y = temp_y;
                    }
                    ct_val = 0;
                    que_state = FALL_INIT; 
					on_player_metatile_collision(tile_start, tile_y, DIR_UP);
                    break;
                } else {
					reset_collision_cache(DIR_UP);
				}
                tile_start++;
            }
            PLAYER.pos.y = new_y;
        }
    }

    //Actor Collisions
    gotoActorCol:
    {
        deltaX = 0;
        deltaY = 0;
        actor_t *hit_actor;
        hit_actor = actor_overlapping_player(FALSE);
        if (hit_actor != NULL && hit_actor->collision_group) {
            //Solid Actors
            if (hit_actor->collision_group == plat_solid_group){
                if(!actor_attached || hit_actor != last_actor){
                    if (temp_y < (hit_actor->pos.y + (hit_actor->bounds.top << 4)) && pl_vel_y >= 0){
                        //Attach to MP
                        last_actor = hit_actor;
                        mp_last_x = hit_actor->pos.x;
                        mp_last_y = hit_actor->pos.y;
                        PLAYER.pos.y = hit_actor->pos.y + (hit_actor->bounds.top << 4) - (PLAYER.bounds.bottom << 4) - 4;
                        //Other cleanup
                        pl_vel_y = 0;
                        actor_attached = TRUE;                        
                        que_state = GROUND_INIT;
                        //PLAYER bounds top seems to be 0 and counting down...
                    } else if (temp_y + (PLAYER.bounds.top << 4) > hit_actor->pos.y + (hit_actor->bounds.bottom<<4)){
                        deltaY += (hit_actor->pos.y - PLAYER.pos.y) + ((-PLAYER.bounds.top + hit_actor->bounds.bottom)<<4) + 32;
                        pl_vel_y = plat_grav;

                        if(que_state == JUMP_STATE || actor_attached){
                            que_state = FALL_INIT;
                        }

                    } else if (PLAYER.pos.x < hit_actor->pos.x){
                        deltaX = (hit_actor->pos.x - PLAYER.pos.x) - ((PLAYER.bounds.right + -hit_actor->bounds.left)<<4);
                        col = 1;
                        last_wall = 1;
                        if(!INPUT_RIGHT){
                            pl_vel_x = 0;
                        }
                    } else if (PLAYER.pos.x > hit_actor->pos.x){
                        deltaX = (hit_actor->pos.x - PLAYER.pos.x) + ((-PLAYER.bounds.left + hit_actor->bounds.right)<<4)+16;
                        col = -1;
                        last_wall = -1;
                        if (!INPUT_LEFT){
                            pl_vel_x = 0;
                        }
                    }

                }
            } else if (hit_actor->collision_group == plat_mp_group){
                //Platform Actors
                if(!actor_attached || hit_actor != last_actor){
                    if (temp_y < hit_actor->pos.y + (hit_actor->bounds.top << 4) && pl_vel_y >= 0){
                        //Attach to MP
                        last_actor = hit_actor;
                        mp_last_x = hit_actor->pos.x;
                        mp_last_y = hit_actor->pos.y;
                        PLAYER.pos.y = hit_actor->pos.y + (hit_actor->bounds.top << 4) - (PLAYER.bounds.bottom << 4) - 4;
                        //Other cleanup
                        pl_vel_y = 0;
                        actor_attached = TRUE;                        
                        que_state = GROUND_INIT;
                    }
                }
            }
            //All Other Collisions
            player_register_collision_with(hit_actor);
        } else if (INPUT_PRESSED(INPUT_PLATFORM_INTERACT)) {
            if (!hit_actor) {
                hit_actor = actor_in_front_of_player(8, TRUE);
            }
            if (hit_actor && !hit_actor->collision_group && hit_actor->script.bank) {
                script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 0);
            }
        }
    }
    
    //ANIMATION--------------------------------------------------------------------------------------------------
    //This animation is currently shared by jumping, dashing, and falling. Dashing doesn't need this complexity though.
    //Here velocity overrides direction. Whereas on the ground it is the reverse. 
    if(plat_turn_control){
        if (INPUT_LEFT){
            PLAYER.dir = DIR_LEFT;
        } else if (INPUT_RIGHT){
            PLAYER.dir = DIR_RIGHT;
        } else if (pl_vel_x < 0) {
            PLAYER.dir = DIR_LEFT;
        } else if (pl_vel_x > 0) {
            PLAYER.dir = DIR_RIGHT;
        }
    }

    if (PLAYER.dir == DIR_LEFT){
        actor_set_anim(&PLAYER, ANIM_JUMP_LEFT);
    } else {
        actor_set_anim(&PLAYER, ANIM_JUMP_RIGHT);
    }


    //STATE CHANGE------------------------------------------------------------------------------------------------
    //Above: FALL -> GROUND in basic_y_col()
	
	//FALL -> SWIM check
	if (script_memory[VAR_CURRENTLEVELTYPE] == 3 && PLAYER.pos.y > 768){
		que_state = SWIM_INIT;
	}
    
    //FALL -> JUMP check 
    if (INPUT_PRESSED(INPUT_PLATFORM_JUMP)){
        if (ct_val != 0){
        //Coyote Time Jump
            que_state = JUMP_INIT;
        } else {
        // Setting the Jump Buffer when jump is pressed while not on the ground
        jb_val = plat_buffer_max; 
        }
    } 
		
	//Pit check
	if (specific_events[FELL_IN_PIT_EVENT].script_addr != 0 && PLAYER.pos.y > 2560 && PLAYER.pos.y < 5120){
		script_execute(specific_events[FELL_IN_PIT_EVENT].script_bank, specific_events[FELL_IN_PIT_EVENT].script_addr, 0, 0);
		que_state = BLANK_INIT;
	}

    //Check for final frame
    if (que_state != FALL_STATE){
        plat_state = FALL_END;
    }
    
    //COUNTERS
    // Counting down Jump Buffer Window
    // Set in Fall and checked in Ground state
    if (jb_val != 0){
        jb_val -= 1;
    }

    // Counting down Coyote Time Window
    // Set in ground and checked in fall state
    if (ct_val != 0 && que_state != GROUND_STATE){
        ct_val -= 1;
    }
    // Counting down the drop-through floor frames
    // XX Checked in Fall, Wall, Ground, and basic_y_col, set in basic_y_col
    if (nocollide != 0){
        nocollide -= 1;
    }

    //FUNCTION TRIGGERS
    trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, INPUT_UP_PRESSED);

    //COUNTERS===============================================================

    //Hone Camera after the player has dashed
    if (camera_deadzone_x > plat_camera_deadzone_x){
        camera_deadzone_x -= 1;
    }

    //State-Based Events
    if(state_events[plat_state].script_addr != 0){
        script_execute(state_events[plat_state].script_bank, state_events[plat_state].script_addr, 0, 0);
    }
}

void swim_state(void) BANKED {
	//INITIALIZE VARS
    UBYTE tile_y = ((PLAYER.pos.y >> 4) + PLAYER.bounds.top + 1) >> 3;
    col = 0;
    
	//Vertical Movement-------------------------------------------------------------------------------------------
    //Add jump force during each jump frame
    if (hold_jump_val !=0){
        //Add the boost per frame amount.
        pl_vel_y -= (jump_per_frame >> 1);
        hold_jump_val -=1;
    } else {
        //Water gravity
		pl_vel_y += plat_grav >> 2;
    }
    
    pl_vel_y = MIN(pl_vel_y,plat_max_fall_vel  >> 2);

    //Collision ---------------------------------------------------------------------------------------------------
    //Vertical Collision Checks
    deltaY += pl_vel_y >> 8;
    WORD temp_y = PLAYER.pos.y;    

    //Horizontal Movement----------------------------------------------------------------------------------------

    //C. ACCELERATION
    if (INPUT_LEFT || INPUT_RIGHT){
        BYTE dir = 1;
        if (INPUT_LEFT){
            dir = -1;
            pl_vel_x = -pl_vel_x;
        }

        if (INPUT_PLATFORM_RUN){
            //Type 1: Smooth Acceleration as the Default in GBStudio
            pl_vel_x = CLAMP(pl_vel_x + plat_run_acc, plat_min_vel >> 1, plat_run_vel >> 1);
            pl_vel_x *= dir;
            deltaX += pl_vel_x >> 8;
            run_stage = 1;
        } else {
            //Ordinay Walk
            if(pl_vel_x < 0 && plat_turn_acc != 0){
                pl_vel_x += plat_turn_acc;
                run_stage = -1;
            } else {
                run_stage = 0;
                pl_vel_x += plat_walk_acc;
                pl_vel_x = CLAMP(pl_vel_x, plat_min_vel >> 1, plat_walk_vel >> 1); 
            }
            pl_vel_x *= dir;
            deltaX += pl_vel_x >> 8;

        }
    } else{
        //DECELERATION
        if (pl_vel_x < 0) {
            pl_vel_x += plat_air_dec;
            if (pl_vel_x > 0) {
                pl_vel_x = 0;
            }
        } else if (pl_vel_x > 0) {
            pl_vel_x -= plat_air_dec; 
            if (pl_vel_x < 0) {
                pl_vel_x = 0;
            }
        }
        run_stage = 0;
        deltaX += pl_vel_x >> 8;
    }

    //D. COLLISIONS
    //X-COLLISION
    gotoXCol:
    {
        deltaX = CLAMP(deltaX, -127, 127);
        UBYTE tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
        UBYTE tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;       
        UWORD new_x = PLAYER.pos.x + deltaX;
        
        UBYTE tile_x = 0;
        UBYTE col_mid = 0;
        
        //Edge Locking
        //If the player is past the right edge (camera or screen)
        if (new_x > (*edge_right + SCREEN_WIDTH - 16) <<4){
            //If the player is trying to go FURTHER right
            if (new_x > PLAYER.pos.x){
                new_x = PLAYER.pos.x;
                pl_vel_x = 0;
            } else {
            //If the player is already off the screen, push them back
                new_x = PLAYER.pos.x - MIN(PLAYER.pos.x - ((*edge_right + SCREEN_WIDTH - 16)<<4), 16);
            }
        //Same but for left side. This side needs a 1 tile (8px) buffer so it doesn't overflow the variable.
        } else if (new_x < *edge_left << 4){
            if (deltaX < 0){
                new_x = PLAYER.pos.x;
                pl_vel_x = 0;
            } else {
                new_x = PLAYER.pos.x + MIN(((*edge_left+8)<<4)-PLAYER.pos.x, 16);
            }
        }

        //Step-Check for collisions one tile left or right for each avatar height tile
        if (new_x > PLAYER.pos.x) {
            tile_x = ((new_x >> 4) + PLAYER.bounds.right) >> 3;
			switch(sram_map_data[VRAM_OFFSET(tile_x, tile_end - 1)]){
				case 151: //beanstalk tile
					que_state = CLIMB_INIT; 
					current_vine_tile_x = tile_x;
					actor_set_dir(&PLAYER, DIR_LEFT, FALSE);
					break;
			}
            
            while (tile_start < tile_end) {
               
                col = tile_at(tile_x, tile_start);
                if (col & COLLISION_LEFT) {				
					new_x = (((tile_x << 3) - PLAYER.bounds.right) << 4) - 1;
					pl_vel_x = 0;
					col = 1;
					last_wall = 1;
					if (grounded){					
						switch(sram_map_data[VRAM_OFFSET(tile_x, tile_start)]){
							case 61: //top part of right pipe
							case 62: //bottom part of right pipe
								if (specific_events[ENTER_RIGHT_PIPE_EVENT].script_addr != 0){
									script_execute(specific_events[ENTER_RIGHT_PIPE_EVENT].script_bank, specific_events[ENTER_RIGHT_PIPE_EVENT].script_addr, 0, 0);
								}
								break;
						}
					}
					on_player_metatile_collision(tile_x, tile_start, DIR_RIGHT); 					
					break;
                } else {
					reset_collision_cache(DIR_RIGHT);
				}
                tile_start++;
            }
        } else if (new_x < PLAYER.pos.x) {
            tile_x = ((new_x >> 4) + PLAYER.bounds.left) >> 3;
            switch(sram_map_data[VRAM_OFFSET(tile_x, tile_end - 1)]){
				case 151: //beanstalk tile
					que_state = CLIMB_INIT; 
					current_vine_tile_x = tile_x;
					actor_set_dir(&PLAYER, DIR_RIGHT, FALSE);
					break;
			}
            while (tile_start < tile_end) {
                col = tile_at(tile_x, tile_start);
                if (col & COLLISION_RIGHT) {					
                    
                    new_x = ((((tile_x + 1) << 3) - PLAYER.bounds.left) << 4) + 1;
                    pl_vel_x = 0;
                    col = -1;
                    last_wall = -1;		
					on_player_metatile_collision(tile_x, tile_start, DIR_LEFT); 					
                    break;
                } else {
					reset_collision_cache(DIR_LEFT);
				}
                tile_start++;
            }
        }
        PLAYER.pos.x = new_x;
    }

    //Y-COLLISION
    {
        deltaY = CLAMP(deltaY, -127, 127);

        //New Y Slopes 1
        UBYTE prev_grounded = grounded;
        UWORD old_y = PLAYER.pos.y;
        grounded = FALSE;
        //End New Y Slopes 1

        UBYTE tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
        UBYTE tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
        if (deltaY > 0) {
            //Moving Downward
            WORD new_y = PLAYER.pos.y + deltaY;
            tile_y = ((new_y >> 4) + PLAYER.bounds.bottom) >> 3;

            if (nocollide == 0){
                //Check collisions from left to right with the bottom of the player
                while (tile_start < tile_end) {
                    if (tile_at(tile_start, tile_y) & COLLISION_TOP) {	
                        new_y = ((((tile_y) << 3) - PLAYER.bounds.bottom) << 4) - 1;	
						grounded = true;
						on_player_metatile_collision(tile_start, tile_y, DIR_DOWN);						
                        break;
                    } else {
						reset_collision_cache(DIR_DOWN);
					}
                    tile_start++;
                }
            }
            PLAYER.pos.y = new_y;

        } else if (deltaY < 0) {
            //Moving Upward
            WORD new_y = PLAYER.pos.y + deltaY;
            UBYTE tile_y = (((new_y >> 4) + PLAYER.bounds.top) >> 3);
            while (tile_start < tile_end) {
                if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM) {					
                    new_y = ((((UBYTE)(tile_y + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
                    pl_vel_y = 0;
                    //MP Test: Attempting stuff to stop the player from continuing upward
                    if(actor_attached){
                        temp_y = last_actor->pos.y;
                        if (last_actor->bounds.top > 0){
                            temp_y += last_actor->bounds.top + last_actor->bounds.bottom << 5;
                        }
                        new_y = temp_y;
                    }
                    ct_val = 0; 
					on_player_metatile_collision(tile_start, tile_y, DIR_UP);
                    break;
                } else {
					reset_collision_cache(DIR_UP);
				}
                tile_start++;
            }
            PLAYER.pos.y = new_y;
        }
    }

    //Actor Collisions
    gotoActorCol:
    {
        deltaX = 0;
        deltaY = 0;
        actor_t *hit_actor;
        hit_actor = actor_overlapping_player(FALSE);
        if (hit_actor != NULL && hit_actor->collision_group) {
            //Solid Actors
            if (hit_actor->collision_group == plat_solid_group){
                if(!actor_attached || hit_actor != last_actor){
                    if (temp_y < (hit_actor->pos.y + (hit_actor->bounds.top << 4)) && pl_vel_y >= 0){
                        //Attach to MP
                        last_actor = hit_actor;
                        mp_last_x = hit_actor->pos.x;
                        mp_last_y = hit_actor->pos.y;
                        PLAYER.pos.y = hit_actor->pos.y + (hit_actor->bounds.top << 4) - (PLAYER.bounds.bottom << 4) - 4;
                        //Other cleanup
                        pl_vel_y = 0;
                        actor_attached = TRUE;   
                        //PLAYER bounds top seems to be 0 and counting down...
                    } else if (temp_y + (PLAYER.bounds.top << 4) > hit_actor->pos.y + (hit_actor->bounds.bottom<<4)){
                        deltaY += (hit_actor->pos.y - PLAYER.pos.y) + ((-PLAYER.bounds.top + hit_actor->bounds.bottom)<<4) + 32;
                        pl_vel_y = plat_grav;

                    } else if (PLAYER.pos.x < hit_actor->pos.x){
                        deltaX = (hit_actor->pos.x - PLAYER.pos.x) - ((PLAYER.bounds.right + -hit_actor->bounds.left)<<4);
                        col = 1;
                        last_wall = 1;
                        if(!INPUT_RIGHT){
                            pl_vel_x = 0;
                        }
                    } else if (PLAYER.pos.x > hit_actor->pos.x){
                        deltaX = (hit_actor->pos.x - PLAYER.pos.x) + ((-PLAYER.bounds.left + hit_actor->bounds.right)<<4)+16;
                        col = -1;
                        last_wall = -1;
                        if (!INPUT_LEFT){
                            pl_vel_x = 0;
                        }
                    }

                }
            } else if (hit_actor->collision_group == plat_mp_group){
                //Platform Actors
                if(!actor_attached || hit_actor != last_actor){
                    if (temp_y < hit_actor->pos.y + (hit_actor->bounds.top << 4) && pl_vel_y >= 0){
                        //Attach to MP
                        last_actor = hit_actor;
                        mp_last_x = hit_actor->pos.x;
                        mp_last_y = hit_actor->pos.y;
                        PLAYER.pos.y = hit_actor->pos.y + (hit_actor->bounds.top << 4) - (PLAYER.bounds.bottom << 4) - 4;
                        //Other cleanup
                        pl_vel_y = 0;
                        actor_attached = TRUE;    
                    }
                }
            }
            //All Other Collisions
            player_register_collision_with(hit_actor);
        } else if (INPUT_PRESSED(INPUT_PLATFORM_INTERACT)) {
            if (!hit_actor) {
                hit_actor = actor_in_front_of_player(8, TRUE);
            }
            if (hit_actor && !hit_actor->collision_group && hit_actor->script.bank) {
                script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 0);
            }
        }
    }
    
    //ANIMATION--------------------------------------------------------------------------------------------------
    //This animation is currently shared by jumping, dashing, and falling. Dashing doesn't need this complexity though.
    //Here velocity overrides direction. Whereas on the ground it is the reverse. 

	if (INPUT_PRESSED(INPUT_LEFT)){
		actor_set_dir(&PLAYER, DIR_LEFT, (pl_vel_x != 0)?TRUE:FALSE);		
    } else if (INPUT_PRESSED(INPUT_RIGHT)){
		actor_set_dir(&PLAYER, DIR_RIGHT, (pl_vel_x != 0)?TRUE:FALSE);
    } else if (pl_vel_x != 0) {
        actor_set_anim_moving(&PLAYER);
    } else {
		actor_set_anim_idle(&PLAYER);
	}
	
	if (pl_vel_y < 0){
		PLAYER.anim_tick = 3;
	} else {
		PLAYER.anim_tick = 7;
	}

    //STATE CHANGE------------------------------------------------------------------------------------------------
    
    //SWIM up check 
    if (INPUT_PRESSED(INPUT_PLATFORM_JUMP) && PLAYER.pos.y > 768){		
		que_state = SWIM_INIT;
    } 
	
	//Pit check
	if (specific_events[FELL_IN_PIT_EVENT].script_addr != 0 && PLAYER.pos.y > 2560 && PLAYER.pos.y < 5120){ 
		script_execute(specific_events[FELL_IN_PIT_EVENT].script_bank, specific_events[FELL_IN_PIT_EVENT].script_addr, 0, 0);
		que_state = BLANK_INIT;
	}

    //Check for final frame
    if (que_state != SWIM_STATE){
        plat_state = SWIM_END;
    }
    
    //COUNTERS
    
    // Counting down the drop-through floor frames
    // XX Checked in Fall, Wall, Ground, and basic_y_col, set in basic_y_col
    if (nocollide != 0){
        nocollide -= 1;
    }

    //FUNCTION TRIGGERS
    trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, INPUT_UP_PRESSED);

    //COUNTERS===============================================================

    //Hone Camera after the player has dashed
    if (camera_deadzone_x > plat_camera_deadzone_x){
        camera_deadzone_x -= 1;
    }

    //State-Based Events
    if(state_events[plat_state].script_addr != 0){
        script_execute(state_events[plat_state].script_bank, state_events[plat_state].script_addr, 0, 0);
    }
}

void assign_state_script(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UWORD *slot = VM_REF_TO_PTR(FN_ARG2);
    UBYTE *bank = VM_REF_TO_PTR(FN_ARG1);
    UBYTE **ptr = VM_REF_TO_PTR(FN_ARG0);
    state_events[*slot].script_bank = *bank;
    state_events[*slot].script_addr = *ptr;
}

void clear_state_script(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UWORD *slot = VM_REF_TO_PTR(FN_ARG0);
    state_events[*slot].script_bank = NULL;
    state_events[*slot].script_addr = NULL;
}

void assign_specific_script(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UWORD *slot = VM_REF_TO_PTR(FN_ARG2);
    UBYTE *bank = VM_REF_TO_PTR(FN_ARG1);
    UBYTE **ptr = VM_REF_TO_PTR(FN_ARG0);
    specific_events[*slot].script_bank = *bank;
    specific_events[*slot].script_addr = *ptr;
}

void clear_specific_script(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UWORD *slot = VM_REF_TO_PTR(FN_ARG0);
    specific_events[*slot].script_bank = NULL;
    specific_events[*slot].script_addr = NULL;
}