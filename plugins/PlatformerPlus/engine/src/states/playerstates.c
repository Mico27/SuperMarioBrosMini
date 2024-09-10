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

pStates plat_state;    //Current platformer state
pStates que_state;

WORD plat_test_var = 0;

void ground_state(void) BANKED {
        //INITIALIZE VARS
    WORD temp_y = 0;
    UBYTE tile_y = ((PLAYER.pos.y >> 4) + PLAYER.bounds.top + 1) >> 3;
    UBYTE old_x = 0;
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
    //B. STATE PHASE 1
    //Add X & Y motion from moving platforms
    //Transform velocity into positional data, to keep the precision of the platform's movement
    grounded = true;
    if (actor_attached){
        //If the platform has been disabled, detach the player
        if(last_actor->disabled == TRUE){
            que_state = FALL_INIT;
            actor_attached = FALSE;
        //If the player is off the platform to the right, detach from the platform
        } else if (PLAYER.pos.x + (PLAYER.bounds.left << 4) > last_actor->pos.x + 16 + (last_actor->bounds.right<< 4)) {
            que_state = FALL_INIT;
            actor_attached = FALSE;
        //If the player is off the platform to the left, detach
        } else if (PLAYER.pos.x + 16 + (PLAYER.bounds.right << 4) < last_actor->pos.x + (last_actor->bounds.left << 4)){
            que_state = FALL_INIT;
            actor_attached = FALSE;
        } else{
        //Otherwise, add any change in movement from platform
            deltaX += (last_actor->pos.x - mp_last_x);
            mp_last_x = last_actor->pos.x;
        }

        //If we're on a platform, zero out any other motion from gravity or other sources
        pl_vel_y = 0;
        
        //Add any change from the platform we're standing on
        deltaY += last_actor->pos.y - mp_last_y;

        //We're setting these to the platform's position, rather than the actor so that if something causes the player to
        //detach (like hitting the roof), they won't automatically get re-attached in the subsequent actor collision step.
        mp_last_y = last_actor->pos.y;
        temp_y = last_actor->pos.y;
    } else if (nocollide != 0){
        //If we're dropping through a platform
        pl_vel_y = 7000; //magic number, rough minimum for actually having the player descend through a platform
        temp_y = PLAYER.pos.y;
    } else {
        //Normal gravity
        pl_vel_y += plat_grav;
        temp_y = PLAYER.pos.y;
        que_state = FALL_INIT; //Use this to test for Falling, avoids an If test in YCollision
    }
    // Add Collision Offset from Moving Platforms
    deltaY += pl_vel_y >> 8;


    //FUNCTION ACCELERATION
    if (INPUT_LEFT || INPUT_RIGHT){
        BYTE dir = 1;
        if (INPUT_LEFT){
            dir = -1;
            pl_vel_x = -pl_vel_x;
        }

        if (INPUT_PLATFORM_RUN){
            if(pl_vel_x < 0){
                pl_vel_x += plat_turn_acc;
                run_stage = -1;
            }
            else{
                pl_vel_x = MIN(pl_vel_x + plat_run_acc, plat_run_vel);
                run_stage = 2;
				PLAYER.anim_tick = 3;
            }
            pl_vel_x *= dir;
            deltaX += pl_vel_x >> 8;
        } else {
            //Ordinay Walk
            if(pl_vel_x < 0 && plat_turn_acc != 0){
                pl_vel_x += plat_turn_acc;
                run_stage = -1;
            } else {
                run_stage = 0;
                pl_vel_x += plat_walk_acc;
                pl_vel_x = CLAMP(pl_vel_x, plat_min_vel, plat_walk_vel); 
				PLAYER.anim_tick = 7;
            }
            pl_vel_x *= dir;
            deltaX += pl_vel_x >> 8;

        }
    } else{
        //DECELERATION
        if (pl_vel_x < 0) {
            pl_vel_x += plat_dec;
            if (pl_vel_x > 0) {
                pl_vel_x = 0;
            }
        } else if (pl_vel_x > 0) {
            pl_vel_x -= plat_dec;
            if (pl_vel_x < 0) {
                pl_vel_x = 0;
            }
        }
        run_stage = 0;
        deltaX += pl_vel_x >> 8;
    }

    //FUNCTION X COLLISION
    {
        deltaX = CLAMP(deltaX, -127, 127);
        old_x = PLAYER.pos.x;
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

    //FUNCTION Y COLLISION
    {
        deltaY = CLAMP(deltaY, -127, 127);

        UBYTE tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
        UBYTE tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
		UBYTE is_leftmost = 1;
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
						switch(plat_state){
							case GROUND_STATE:
								que_state = GROUND_STATE; 
								pl_vel_y = 256;
								break;
							case GROUND_INIT:
								que_state = GROUND_STATE;
								break;							
							default:
								que_state = GROUND_INIT;
								break;								
						}						
						on_player_metatile_collision(tile_start, tile_y, DIR_DOWN); 
                        break;
                    } else {
						reset_collision_cache(DIR_DOWN);
					}
                    tile_start++;
					is_leftmost = 0;
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
                    actor_attached = FALSE;
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
    
    //ANIMATION---------------------------------------------------------------------------------------------------
    //Button direction overrides velocity, for slippery run reasons
    if (INPUT_LEFT){
		if (pl_vel_x > 0) {
			que_state = SKID_INIT;
		} else if (plat_state == SKID_STATE) {
			que_state = GROUND_INIT;
		}
		actor_set_dir(&PLAYER, DIR_LEFT, TRUE);
    } else if (INPUT_RIGHT){
		if (pl_vel_x < 0) {
			que_state = SKID_INIT;
		} else if (plat_state == SKID_STATE) {
			que_state = GROUND_INIT;
		}
        actor_set_dir(&PLAYER, DIR_RIGHT, TRUE);
    } else if (pl_vel_x < 0) {
        actor_set_dir(&PLAYER, DIR_LEFT, TRUE);
    } else if (pl_vel_x > 0) {
        actor_set_dir(&PLAYER, DIR_RIGHT, TRUE);
    } else {
        actor_set_anim_idle(&PLAYER);
    }


    //STATE CHANGE: Above, basic_y_col can shift to FALL_STATE.--------------------------------------------------
    
    //GROUND -> JUMP Check
    if (INPUT_PRESSED(INPUT_PLATFORM_JUMP) || jb_val != 0){
        if (nocollide == 0){
            //Standard Jump
            que_state = JUMP_INIT;

        }
    }
    jb_val = 0;
	
	//GROUND -> Crouch
	if (INPUT_DOWN){
		que_state = CROUCH_INIT;
	}

    //Check for final frame
    if (que_state != GROUND_STATE){
        plat_state = GROUND_END;
    }

    //COUNTERS
    // Counting down the drop-through floor frames
    // XX Checked in Fall, Wall, Ground, and basic_y_col, set in basic_y_col
    if (nocollide != 0){
        nocollide -= 1;
    }

    gotoTriggerCol:
    //FUNCTION TRIGGERS
    trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, INPUT_UP_PRESSED);

    gotoCounters:
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

void crouch_state(void) BANKED {
        //INITIALIZE VARS
    WORD temp_y = 0;
    UBYTE tile_y = ((PLAYER.pos.y >> 4) + PLAYER.bounds.top + 1) >> 3;
    UBYTE old_x = 0;
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
    //B. STATE PHASE 1
    //Add X & Y motion from moving platforms
    //Transform velocity into positional data, to keep the precision of the platform's movement
    grounded = true;
    if (actor_attached){
        //If the platform has been disabled, detach the player
        if(last_actor->disabled == TRUE){
            que_state = FALL_INIT;
            actor_attached = FALSE;
        //If the player is off the platform to the right, detach from the platform
        } else if (PLAYER.pos.x + (PLAYER.bounds.left << 4) > last_actor->pos.x + 16 + (last_actor->bounds.right<< 4)) {
            que_state = FALL_INIT;
            actor_attached = FALSE;
        //If the player is off the platform to the left, detach
        } else if (PLAYER.pos.x + 16 + (PLAYER.bounds.right << 4) < last_actor->pos.x + (last_actor->bounds.left << 4)){
            que_state = FALL_INIT;
            actor_attached = FALSE;
        } else{
        //Otherwise, add any change in movement from platform
            deltaX += (last_actor->pos.x - mp_last_x);
            mp_last_x = last_actor->pos.x;
        }

        //If we're on a platform, zero out any other motion from gravity or other sources
        pl_vel_y = 0;
        
        //Add any change from the platform we're standing on
        deltaY += last_actor->pos.y - mp_last_y;

        //We're setting these to the platform's position, rather than the actor so that if something causes the player to
        //detach (like hitting the roof), they won't automatically get re-attached in the subsequent actor collision step.
        mp_last_y = last_actor->pos.y;
        temp_y = last_actor->pos.y;
    } else if (nocollide != 0){
        //If we're dropping through a platform
        pl_vel_y = 7000; //magic number, rough minimum for actually having the player descend through a platform
        temp_y = PLAYER.pos.y;
    } else {
        //Normal gravity
        pl_vel_y += plat_grav;
        temp_y = PLAYER.pos.y;
        que_state = FALL_INIT; //Use this to test for Falling, avoids an If test in YCollision
    }
    // Add Collision Offset from Moving Platforms
    deltaY += pl_vel_y >> 8;

    //DECELERATION
    if (pl_vel_x < 0) {
        pl_vel_x += plat_dec;
        if (pl_vel_x > 0) {
            pl_vel_x = 0;
        }
    } else if (pl_vel_x > 0) {
        pl_vel_x -= plat_dec;
        if (pl_vel_x < 0) {
            pl_vel_x = 0;
        }
    }
    run_stage = 0;
    deltaX += pl_vel_x >> 8;

    //FUNCTION X COLLISION
    {
        deltaX = CLAMP(deltaX, -127, 127);
        old_x = PLAYER.pos.x;
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

    //FUNCTION Y COLLISION
    {
        deltaY = CLAMP(deltaY, -127, 127);

        UBYTE tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
        UBYTE tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
		UBYTE is_leftmost = 1;
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
						switch(plat_state){
							case CROUCH_STATE:
								que_state = CROUCH_STATE; 
								pl_vel_y = 256;
								break;
							case CROUCH_INIT:
								if (is_leftmost == 1){//check only mario's left leg for left pipe part
									UBYTE tile_id = sram_map_data[VRAM_OFFSET(tile_start, tile_y)];
									if (tile_id == 57 && specific_events[ENTER_DOWN_PIPE_EVENT].script_addr != 0){
										script_execute(specific_events[ENTER_DOWN_PIPE_EVENT].script_bank, specific_events[ENTER_DOWN_PIPE_EVENT].script_addr, 0, 0);
									}
								}
								que_state = CROUCH_STATE;
								break;
							default:
								que_state = CROUCH_INIT;
								break;								
						}						
						on_player_metatile_collision(tile_start, tile_y, DIR_DOWN); 
                        break;
                    } else {
						reset_collision_cache(DIR_DOWN);
					}
                    tile_start++;
					is_leftmost = 0;
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
                    actor_attached = FALSE;
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
                        que_state = CROUCH_INIT;
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
                        que_state = CROUCH_INIT;
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
    
    //ANIMATION---------------------------------------------------------------------------------------------------
    //Button direction overrides velocity, for slippery run reasons
    if (INPUT_LEFT){
		actor_set_dir(&PLAYER, DIR_LEFT, FALSE);
    } else if (INPUT_RIGHT){
        actor_set_dir(&PLAYER, DIR_RIGHT, FALSE);
    } else {
        actor_set_anim_idle(&PLAYER);
    }


    //STATE CHANGE: Above, basic_y_col can shift to FALL_STATE.--------------------------------------------------
    
    //CROUCH -> JUMP Check
    if (INPUT_PRESSED(INPUT_PLATFORM_JUMP) || jb_val != 0){
        if (nocollide == 0){
            //Standard Jump
            que_state = JUMP_INIT;

        }
    }
    jb_val = 0;
	
	//GROUND -> Crouch
	if (!INPUT_DOWN){
		que_state = GROUND_INIT;
	}

    //Check for final frame
    if (que_state != CROUCH_STATE){
        plat_state = CROUCH_END;
    }

    //COUNTERS
    // Counting down the drop-through floor frames
    // XX Checked in Fall, Wall, Ground, and basic_y_col, set in basic_y_col
    if (nocollide != 0){
        nocollide -= 1;
    }

    gotoTriggerCol:
    //FUNCTION TRIGGERS
    trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, INPUT_UP_PRESSED);

    gotoCounters:
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

void jump_state(void) BANKED {
    //INITIALIZE VARS
    WORD temp_y = 0;
    UBYTE tile_y = ((PLAYER.pos.y >> 4) + PLAYER.bounds.top + 1) >> 3;
    UBYTE old_x = 0;
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


    //Vertical Movement-------------------------------------------------------------------------------------------
    //Add jump force during each jump frame
    if (hold_jump_val !=0 && (INPUT_PLATFORM_JUMP || enemy_bounce)){
        //Add the boost per frame amount.
        pl_vel_y -= jump_per_frame;
        //Reduce subsequent jump amounts (for double jumps)
        if (plat_jump_vel >= jump_reduction_val){
            pl_vel_y += jump_reduction_val;
        } else {
            //When reducing that value, zero out if it's negative
            pl_vel_y = 0;
        }
        //Add jump boost from horizontal movement and/or enemy bounce
        WORD tempBoost = (pl_vel_x >> 8) * (enemy_bounce + 1) * boost_val;
		
        //Take the positive value of x-vel
        tempBoost = MAX(tempBoost, -tempBoost);
        //This is a test to see if the results will overflow pl_vel_y. Note, pl_vel_y is negative here.
        if (tempBoost > 32767 + pl_vel_y){
            pl_vel_y = -32767;
        }
        else{
            pl_vel_y += -tempBoost;
        }
        hold_jump_val -=1;
    } else if (INPUT_PLATFORM_JUMP  && pl_vel_y < 0){
        //After the jump frames end, use the reduced gravity
        pl_vel_y += plat_hold_grav;
    } else if (pl_vel_y >= 0){
        que_state = FALL_INIT;
        actor_attached = FALSE;
        pl_vel_y += plat_grav;
    } else {
        pl_vel_y += plat_grav;
    }

    temp_y = PLAYER.pos.y;
    //Start DeltaX with Actor offsets
    deltaY += pl_vel_y >> 8;

    //Horizontal Movement-----------------------------------------------------------------------------------------
    
    //FUNCTION ACCELERATION
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

    //FUNCTION X COLLISION

    gotoXCol:
    {
        deltaX = CLAMP(deltaX, -127, 127);
        old_x = PLAYER.pos.x;
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

            while (tile_start < tile_end) {
               
                col = tile_at(tile_x, tile_start);
                if (col & COLLISION_LEFT) {
                    
					new_x = (((tile_x << 3) - PLAYER.bounds.right) << 4) - 1;
					pl_vel_x = 0;
					col = 1;
					last_wall = 1;
					break;
                }
                tile_start++;
            }
        } else if (new_x < PLAYER.pos.x) {
            tile_x = ((new_x >> 4) + PLAYER.bounds.left) >> 3;           

            while (tile_start < tile_end) {
                col = tile_at(tile_x, tile_start);

                if (col & COLLISION_RIGHT) {
                    
                    new_x = ((((tile_x + 1) << 3) - PLAYER.bounds.left) << 4) + 1;
                    pl_vel_x = 0;
                    col = -1;
                    last_wall = -1;
                    break;
                }
                tile_start++;
            }
        }
        PLAYER.pos.x = new_x;
    }

    gotoYCol:
    {
        //FUNCTION Y COLLISION
        deltaY = CLAMP(deltaY, -127, 127);

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
                            pl_vel_y = 256;
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
                    actor_attached = FALSE;
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

    //FUNCTION ACTOR CHECK
    //Actor Collisions
    gotoActorColJump:
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
   
    //ANIMATION---------------------------------------------------------------------------------------------------
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
    //Above: JUMP-> NEUTRAL when a) player starts descending, b) player hits roof, c) player stops pressing, d)jump frames run out.
  

    //Check for final frame
    if (que_state != JUMP_STATE){
        plat_state = JUMP_END;
    }

    gotoTriggerCol:
    //FUNCTION TRIGGERS
    trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, INPUT_UP_PRESSED);

    gotoCounters:
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