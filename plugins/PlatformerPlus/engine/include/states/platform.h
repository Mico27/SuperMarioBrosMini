#ifndef STATE_PLATFORM_H
#define STATE_PLATFORM_H

#include <gb/gb.h>

#define ANIM_STATE_DEFAULT 0
#define ANIM_STATE_KNOCKED 1
#define ANIM_STATE_DEATH 2
#define ANIM_STATE_TUCKED 3
#define ANIM_STATE_SWIM 4
#define ANIM_STATE_CHANGESIZE 5
#define ANIM_STATE_SKID 6
#define ANIM_STATE_CLIMB 7
#define ANIM_STATE_CROUCH 8
#define ANIM_STATE_PIPETRANSITION 9
#define ANIM_STATE_STATIC 10
#define ANIM_STATE_1UP 11
#define ANIM_STATE_RED 12
#define ANIM_STATE_RED_KNOCKED 13
#define ANIM_STATE_SCORE20 14
#define ANIM_STATE_SCORE40 15
#define ANIM_STATE_SCORE80 16
#define ANIM_STATE_SCORE100 17
#define ANIM_STATE_SCORE200 18
#define ANIM_STATE_MUSHROOM 19
#define ANIM_STATE_FLOWER 20
#define ANIM_STATE_STAR 21
#define ANIM_STATE_COINBLOCK 22
#define ANIM_STATE_COINBRICK 23
#define ANIM_STATE_EMPTYBLOCK 24
#define ANIM_STATE_FLOWERBLOCK 25
#define ANIM_STATE_MUSHROOMBLOCK 26
#define ANIM_STATE_STARBLOCK 27
#define ANIM_STATE_FIREBALL 28
#define ANIM_STATE_FIREBALLHIT 29
#define ANIM_STATE_BRICKBREAK 30
#define ANIM_STATE_1UPBLOCK 31
#define ANIM_STATE_FIREBAR 32
#define ANIM_STATE_BEANSTALKBLOCK 33
#define ANIM_STATE_BEANSTALK 34
#define ANIM_STATE_COIN 35
#define ANIM_STATE_WINGED 36
#define ANIM_STATE_RED_TUCKED 37
#define ANIM_STATE_RED_WINGED 38
#define ANIM_STATE_CLOUDPLATFORM 39
#define ANIM_STATE_MEDIUM 40
#define ANIM_STATE_SMALL 41
#define ANIM_STATE_BALANCINGPLATFORM 42
#define ANIM_STATE_SMALLBALANCINGPLATFORM 43

void platform_init(void) BANKED;
void platform_update(void) BANKED;

void check_player_metatiles_entered(void) BANKED;
void on_player_metatile_collision(UBYTE tile_x, UBYTE tile_y, UBYTE direction) BANKED;
void reset_collision_cache(UBYTE direction) BANKED;

typedef struct script_state_t {
    UBYTE script_bank;
    UBYTE *script_addr;
} script_state_t;

typedef enum {              //Specific event types
	COIN_COLLECTED_EVENT = 0,
	HIT_BLOCK_EVENT = 1,
	ENTER_DOWN_PIPE_EVENT = 2,
	ENTER_RIGHT_PIPE_EVENT = 3,
	VINE_WARP_EVENT = 4,
	FELL_IN_PIT_EVENT = 5,
}  pSpecificEvent;

extern script_state_t specific_events[6];

extern WORD pl_vel_x;
extern WORD pl_vel_y;
extern WORD plat_min_vel;
extern WORD plat_walk_vel;
extern WORD plat_run_vel;
extern WORD plat_climb_vel;
extern WORD plat_walk_acc;
extern WORD plat_run_acc;
extern WORD plat_dec;
extern WORD plat_jump_vel;
extern WORD plat_grav;
extern WORD plat_hold_grav;
extern WORD plat_max_fall_vel;

extern BYTE plat_camera_deadzone_x;
extern UBYTE plat_camera_block;
extern UBYTE plat_drop_through;
extern UBYTE plat_mp_group;        
extern UBYTE plat_solid_group;    
extern WORD plat_jump_min; 
extern UBYTE plat_hold_jump_max; 
extern WORD plat_jump_reduction;
extern UBYTE plat_coyote_max;
extern UBYTE plat_buffer_max;    
extern UBYTE plat_float_input;   
extern WORD plat_float_grav;   
extern UBYTE plat_turn_control; 
extern WORD plat_air_dec;    
extern WORD plat_turn_acc;
extern UBYTE plat_run_boost;
extern BYTE run_stage;

extern UBYTE grounded;
extern UBYTE current_vine_tile_x;

#endif
