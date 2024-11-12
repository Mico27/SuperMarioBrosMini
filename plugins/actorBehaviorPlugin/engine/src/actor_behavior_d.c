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
#include "actor_behavior_d.h"
#include "states/platform.h"
#include "states/playerstates.h"
#include "data/states_defines.h"
#include "meta_tiles.h"
#include "collision.h"
#include "data_manager.h"
#include "data/game_globals.h"
#include "ui.h"


const BYTE head_swipe_attack_velx[] = { -8, -4, 0, 16, 4 };
const BYTE hand_swipe_attack_velx[] = { -32, -8, 8, 32, 16 };
const BYTE hand_swipe_attack_vely[] = {   -4, 32, 16,  0,  -4 };

actor_t * right_bowser_hand;
actor_t * left_bowser_hand;
UBYTE right_bowser_hand_idx;
UBYTE left_bowser_hand_idx;
UBYTE debris_idx;

void set_bowser_frame(UBYTE frame_idx) BANKED {
	if (frame_idx != script_memory[VAR_BOWSER_BACKGROUND_IDX]){
		script_memory[VAR_BOWSER_BACKGROUND_IDX] = frame_idx;
		if(specific_events[BOWSER_BACKGROUND_CHANGE_EVENT].script_addr != 0){
			script_execute(specific_events[BOWSER_BACKGROUND_CHANGE_EVENT].script_bank, specific_events[BOWSER_BACKGROUND_CHANGE_EVENT].script_addr, 0, 0);
		}
	}
}

void actor_behavior_update_d(UBYTE i, actor_t * actor) BANKED {
	switch(current_behavior){					
		case 44: //Giant Bowser
		switch(actor_states[i]){
			case 0:
				actor_states[i] = 1; 				
				ui_set_pos((actor->pos.x >> 4) - 24, (actor->pos.y >> 4) - 32);
				set_bowser_frame(3);
				actor_counter_a[i] = 0;
				actor_counter_b[i] = 0;	
				actor_vel_x[i] = 0;
				actor_vel_y[i] = 0;
				actor->collision_enabled = false;
				right_bowser_hand_idx = actor_linked_actor_idx[i];
				left_bowser_hand_idx = actor_linked_actor_idx[right_bowser_hand_idx];
				right_bowser_hand = (actors + right_bowser_hand_idx);
				left_bowser_hand = (actors + left_bowser_hand_idx);	
				actor_set_dir(right_bowser_hand, DIR_UP, FALSE);
				actor_set_dir(left_bowser_hand, DIR_DOWN, FALSE);	
				right_bowser_hand->frame = right_bowser_hand->frame_start + 1;
				left_bowser_hand->frame = left_bowser_hand->frame_start + 1;	
				right_bowser_hand->collision_enabled = false;
				left_bowser_hand->collision_enabled = false;
				break;
			case 1: //Intro
				if (!(game_time & 1)){
					if (actor->pos.y > 1408){
						actor_vel_y[i] = -8;
					} else {
						actor_states[i] = 2; 
						set_bowser_frame(3);
						actor_counter_a[i] = 0;
						actor_counter_b[i] = 0;
						actor_vel_y[i] = 0;
						actor->collision_enabled = true;
						right_bowser_hand->collision_enabled = true;
						left_bowser_hand->collision_enabled = true;
						actor_set_dir(right_bowser_hand, DIR_RIGHT, FALSE);
						actor_set_dir(left_bowser_hand, DIR_LEFT, FALSE);	
						right_bowser_hand->frame = right_bowser_hand->frame_start;
						left_bowser_hand->frame = left_bowser_hand->frame_start;
					}
					actor->pos.y =  actor->pos.y + actor_vel_y[i];
					right_bowser_hand->pos.y =  right_bowser_hand->pos.y + actor_vel_y[i];
					left_bowser_hand->pos.y =  left_bowser_hand->pos.y + actor_vel_y[i];
					
					ui_set_pos((actor->pos.x >> 4) - 24, (actor->pos.y >> 4) - 32);
				}
				break;
			case 2: //Idle
				if (!(game_time & 1)){	
				
					//Head
					if (actor->pos.y < 1088){
						actor_counter_b[i] = 1;					
					} else if (actor->pos.y >= 1216){
						actor_counter_b[i] = 0;
					}
					if (!(game_time & 3)){	
						if (actor->pos.x < 2048 && actor->pos.x < PLAYER.pos.x){
							if (actor_vel_x[i] < 8){
								actor_vel_x[i]++;
							}		
						} else if (actor->pos.x > 512 && actor->pos.x >= PLAYER.pos.x + 256){
							if (actor_vel_x[i] > -8){
								actor_vel_x[i]--;
							}
						} else {
							actor_vel_x[i] = 0;
						}
					}
					if (actor_counter_b[i]){
						if (actor_vel_y[i] < 8){
							actor_vel_y[i]++;
						}
					} else {
						if (actor_vel_y[i] > -8){
							actor_vel_y[i]--;
						}
					}
					actor->pos.x =  actor->pos.x + actor_vel_x[i];
					actor->pos.y =  actor->pos.y + actor_vel_y[i];
					ui_set_pos((actor->pos.x >> 4) - 24, (actor->pos.y >> 4) - 32);
					
					//Right hand
					if (right_bowser_hand->pos.y < 1408){
						actor_counter_b[right_bowser_hand_idx] = 1;					
					} else if (right_bowser_hand->pos.y >= 1536){
						actor_counter_b[right_bowser_hand_idx] = 0;
					}
					if (right_bowser_hand->pos.x < actor->pos.x + 512){
						if (actor_vel_x[right_bowser_hand_idx] < 8){
							actor_vel_x[right_bowser_hand_idx]++;
						}		
					} else if (right_bowser_hand->pos.x >= actor->pos.x + 520){
						if (actor_vel_x[right_bowser_hand_idx] > -8){
							actor_vel_x[right_bowser_hand_idx]--;
						}
					} else {
						actor_vel_x[right_bowser_hand_idx] = 0;
					}
					if (actor_counter_b[right_bowser_hand_idx]){
						if (actor_vel_y[right_bowser_hand_idx] < 8){
							actor_vel_y[right_bowser_hand_idx]++;
						}
					} else {
						if (actor_vel_y[right_bowser_hand_idx] > -8){
							actor_vel_y[right_bowser_hand_idx]--;
						}
					}
					right_bowser_hand->pos.x =  right_bowser_hand->pos.x + actor_vel_x[right_bowser_hand_idx];
					right_bowser_hand->pos.y =  right_bowser_hand->pos.y + actor_vel_y[right_bowser_hand_idx];
					
					//Left hand
					if (left_bowser_hand->pos.y < 1408){
						actor_counter_b[left_bowser_hand_idx] = 1;					
					} else if (left_bowser_hand->pos.y >= 1536){
						actor_counter_b[left_bowser_hand_idx] = 0;
					}
					if (left_bowser_hand->pos.x < actor->pos.x - 512){
						if (actor_vel_x[left_bowser_hand_idx] < 8){
							actor_vel_x[left_bowser_hand_idx]++;
						}		
					} else if (left_bowser_hand->pos.x >= actor->pos.x - 504){
						if (actor_vel_x[left_bowser_hand_idx] > -8){
							actor_vel_x[left_bowser_hand_idx]--;
						}
					} else {
						actor_vel_x[left_bowser_hand_idx] = 0;
					}
					if (actor_counter_b[left_bowser_hand_idx]){
						if (actor_vel_y[left_bowser_hand_idx] < 8){
							actor_vel_y[left_bowser_hand_idx]++;
						}
					} else {
						if (actor_vel_y[left_bowser_hand_idx] > -8){
							actor_vel_y[left_bowser_hand_idx]--;
						}
					}
					left_bowser_hand->pos.x =  left_bowser_hand->pos.x + actor_vel_x[left_bowser_hand_idx];
					left_bowser_hand->pos.y =  left_bowser_hand->pos.y + actor_vel_y[left_bowser_hand_idx];
					
					
					if (actor_counter_a[i] > 96){
						actor_counter_a[i] = rand();
						if (actor_counter_a[i] < 128){
							if (actor->pos.x < 1280){
								actor_counter_a[i] = 0;
								actor_counter_b[i] = 0;
								actor_states[i] = 6; 							
								right_bowser_hand->frame = right_bowser_hand->frame_start + 1;
								break;		
							} else {
								actor_counter_a[i] = 0;
								actor_counter_b[i] = 0;
								actor_states[i] = 5; 							
								left_bowser_hand->frame = left_bowser_hand->frame_start + 1;
								break;
							}
							
						} else {
							actor_counter_a[i] = 0;
							actor_counter_b[i] = 0;
							actor_states[i] = 7;
							actor_vel_y[i] = 0;
							break;
						}						
					}
					actor_counter_a[i]++;
					set_bowser_frame(3);
					
				}
				break;
			case 3: //hurt init
				actor_counter_a[i] = 16;
				actor_states[i] = 4; 
				actor_vel_y[i] = 8;
				set_bowser_frame(1);
				actor->collision_enabled = false;
				right_bowser_hand->frame = right_bowser_hand->frame_start + 1;
				left_bowser_hand->frame = left_bowser_hand->frame_start + 1;
			case 4: //hurt				
				if (!(game_time & 3)){	
					actor_vel_y[i]--;				
					actor_counter_a[i]--;					
					if (actor_counter_a[i] <= 0){
						actor_states[i] = 2;
						set_bowser_frame(3);
						actor->collision_enabled = true;	
						right_bowser_hand->frame = right_bowser_hand->frame_start;
						left_bowser_hand->frame = left_bowser_hand->frame_start;						
					}				
				}				
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				ui_set_pos((actor->pos.x >> 4) - 24, (actor->pos.y >> 4) - 32);
				break;
			case 5: //Left swipe attack
			case 6: //Right swipe attack
				if (!(game_time & 15)){					
					if (actor->pos.y < 1280){
						actor_counter_b[i] = 1;					
					} else if (actor->pos.y >= 1408){
						actor_counter_b[i] = 0;
					}
					if (actor_counter_b[i]){
						actor_vel_y[i] = 8;
					} else {
						actor_vel_y[i] = -8;
					}
					if (actor_counter_a[i] < 5){						
						if (actor_states[i] == 6){
							actor_vel_x[i] = -head_swipe_attack_velx[actor_counter_a[i]];						
							actor_vel_x[right_bowser_hand_idx] = -hand_swipe_attack_velx[actor_counter_a[i]];
							actor_vel_y[right_bowser_hand_idx] = hand_swipe_attack_vely[actor_counter_a[i]];
						} else {
							actor_vel_x[i] = head_swipe_attack_velx[actor_counter_a[i]];						
							actor_vel_x[left_bowser_hand_idx] = hand_swipe_attack_velx[actor_counter_a[i]];
							actor_vel_y[left_bowser_hand_idx] = hand_swipe_attack_vely[actor_counter_a[i]];
						}
						actor_counter_a[i]++;
					} else {
						actor_states[i] = 2;
						actor_counter_a[i] = 0;
						right_bowser_hand->frame = right_bowser_hand->frame_start;
						left_bowser_hand->frame = left_bowser_hand->frame_start;
					}	
				}
				actor->pos.y =  actor->pos.y + actor_vel_y[i];
				actor->pos.x =  actor->pos.x + actor_vel_x[i];
				ui_set_pos((actor->pos.x >> 4) - 24, (actor->pos.y >> 4) - 32);
				if (actor_states[i] == 6){
					right_bowser_hand->pos.x =  right_bowser_hand->pos.x + actor_vel_x[right_bowser_hand_idx];
					right_bowser_hand->pos.y =  right_bowser_hand->pos.y + actor_vel_y[right_bowser_hand_idx];
				} else {
					left_bowser_hand->pos.x =  left_bowser_hand->pos.x + actor_vel_x[left_bowser_hand_idx];
					left_bowser_hand->pos.y =  left_bowser_hand->pos.y + actor_vel_y[left_bowser_hand_idx];	
				}
				break;
			case 7: //Slam attack				
				if (right_bowser_hand->pos.y < 1280 && left_bowser_hand->pos.y < 1280){
					actor_counter_b[i] = 1;					
				}				
				if (actor_counter_b[i]){
					if (actor_vel_y[i] < 64){
						actor_vel_y[i]++;
					}
					if (right_bowser_hand->pos.y > 2048 && left_bowser_hand->pos.y > 2048){
						actor_states[i] = 8;
						actor_vel_y[i] = 0;
						actor_counter_b[i] = 0;
						if(specific_events[EARTHQUAKE_EVENT].script_addr != 0){
							script_execute(specific_events[EARTHQUAKE_EVENT].script_bank, specific_events[EARTHQUAKE_EVENT].script_addr, 0, 0);
						}
						break;
					}
					if (right_bowser_hand->pos.y <= 2048){
						right_bowser_hand->pos.y =  right_bowser_hand->pos.y + actor_vel_y[i];	
					}
					if (left_bowser_hand->pos.y <= 2048){
						left_bowser_hand->pos.y =  left_bowser_hand->pos.y + actor_vel_y[i];	
					}
				} else {
					if (actor_vel_y[i] > -4){
						actor_vel_y[i]--;
					}
					if (right_bowser_hand->pos.y >= 1280){
						right_bowser_hand->pos.y =  right_bowser_hand->pos.y + actor_vel_y[i];	
					}
					if (left_bowser_hand->pos.y >= 1280){
						left_bowser_hand->pos.y =  left_bowser_hand->pos.y + actor_vel_y[i];	
					}
				}
				break;
			case 8: //Debris fall
				//debris 1							
				debris_idx = actor_linked_actor_idx[left_bowser_hand_idx];
				if (actor_states[debris_idx] != 0 && actor_states[debris_idx] != 255){
					//debris 2
					debris_idx = actor_linked_actor_idx[debris_idx];
					if (actor_states[debris_idx] != 0 && actor_states[debris_idx] != 255){
						//debris 3
						debris_idx = actor_linked_actor_idx[debris_idx];
						if (actor_states[debris_idx] != 0 && actor_states[debris_idx] != 255){
							//debris 4
							debris_idx = actor_linked_actor_idx[debris_idx];
							actor_states[i] = 2;
						}
					}
				}		
				if (debris_idx != 0 && (actor_states[debris_idx] == 0 || actor_states[debris_idx] == 255)){
					actor_t * debris_actor = (actors + debris_idx);
					actor_states[debris_idx] = 0;
					if (!debris_actor->active){
						debris_actor->disabled = FALSE;
						activate_actor(debris_actor);
					}
					debris_actor->collision_enabled = true;
					debris_actor->pos.x = actor->pos.x + (((rand() & 127) - 64) << 4);
					debris_actor->pos.y = 0;	
					actor_vel_x[debris_idx] = 0;
					actor_vel_y[debris_idx] = 0;
					if ((((debris_actor->pos.x >> 4) + 8) - draw_scroll_x) < 80){
						actor_set_dir(debris_actor, DIR_RIGHT, TRUE);						
					} else {
						actor_set_dir(debris_actor, DIR_LEFT, TRUE);
					}
				}
				break;
			case 9: //init death
				//debris 1							
				debris_idx = actor_linked_actor_idx[left_bowser_hand_idx];
				actor_t * debris_actor = (actors + debris_idx);
				deactivate_actor(debris_actor);
				//debris 2
				debris_idx = actor_linked_actor_idx[debris_idx];
				debris_actor = (actors + debris_idx);
				deactivate_actor(debris_actor);
				//debris 3
				debris_idx = actor_linked_actor_idx[debris_idx];
				debris_actor = (actors + debris_idx);
				deactivate_actor(debris_actor);
				//debris 4
				debris_idx = actor_linked_actor_idx[debris_idx];
				debris_actor = (actors + debris_idx);
				deactivate_actor(debris_actor);
				set_bowser_frame(2);
				actor->collision_enabled = false;
				right_bowser_hand->collision_enabled = false;
				left_bowser_hand->collision_enabled = false;
				right_bowser_hand->frame = right_bowser_hand->frame_start + 1;
				left_bowser_hand->frame = left_bowser_hand->frame_start + 1;
				actor_states[i] = 10;
			case 10: //Dead
				if (actor->pos.y > 2560){
					actor_states[i] = 255;
				} else {	
					actor->pos.y =  actor->pos.y + 4;
					right_bowser_hand->pos.y =  right_bowser_hand->pos.y + 4;
					left_bowser_hand->pos.y =  left_bowser_hand->pos.y + 4;
					ui_set_pos((actor->pos.x >> 4) - 24, (actor->pos.y >> 4) - 32);
				}
			break;
			case 255: //Deactivate
				deactivate_actor(actor);
				deactivate_actor(right_bowser_hand);
				deactivate_actor(left_bowser_hand);
				ui_set_pos(0, 144);
				break;
		}		
		break;
	}			
}
