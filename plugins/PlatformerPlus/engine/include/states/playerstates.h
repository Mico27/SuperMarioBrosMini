#include "states/platform.h"


void fall_state(void) BANKED;
void ground_state(void) BANKED;
void crouch_state(void) BANKED;
void jump_state(void) BANKED;
//void vine_state(void) BANKED;
void blank_state(void) BANKED;

typedef enum {              //Datatype for tracking states
	START_LEVEL_INIT = 0,
	START_LEVEL_STATE = 1,
	START_LEVEL_END = 2,
    FALL_INIT = 3,
    FALL_STATE = 4,
    FALL_END = 5,
    GROUND_INIT = 6,
    GROUND_STATE = 7,
    GROUND_END = 8,
	CROUCH_INIT = 9,
	CROUCH_STATE = 10,
	CROUCH_END = 11,
	SKID_INIT = 12,
	SKID_STATE = 13,
	SKID_END = 14,
    JUMP_INIT = 15,
    JUMP_STATE = 16,
    JUMP_END = 17,
    VINE_INIT = 18,
    VINE_STATE = 19,
    VINE_END = 20,
    PIPE_INIT = 21,
    PIPE_STATE = 22,
    PIPE_END = 23,
    POLE_INIT = 24,
    POLE_STATE = 25,
    POLE_END = 26,
    CHANGESIZE_INIT = 27,
    CHANGESIZE_STATE = 28,
	CHANGESIZE_END = 29,
	DEAD_INIT = 30,
	DEAD_STATE = 31,
	DEAD_END = 32,
	FIREFLOWER_INIT = 33,
	FIREFLOWER_STATE = 34,
	FIREFLOWER_END = 35,
	END_LEVEL_INIT = 36,
	END_LEVEL_STATE = 37,
	END_LEVEL_END = 38,
    BLANK_INIT = 39,
    BLANK_STATE = 40
}  pStates;

extern pStates plat_state;
extern pStates que_state;
extern script_state_t state_events[40];
       
extern UBYTE nocollide;            
extern WORD deltaX;
extern WORD deltaY;

extern UBYTE ct_val;               //Coyote Time Variable
extern UBYTE jb_val;               //Jump Buffer Variable
extern UBYTE hold_jump_val;        //Jump input hold variable

extern actor_t *last_actor;        //The last actor the player hit, and that they were attached to
extern UBYTE actor_attached;       //Keeps track of whether the player is currently on an actor and inheriting its movement
extern WORD mp_last_x;             //Keeps track of the pos.x of the attached actor from the previous frame
extern WORD mp_last_y;             //Keeps track of the pos.y of the attached actor from the previous frame

extern WORD jump_reduction_val;    //Holds a temporary jump velocity reduction
extern WORD jump_per_frame;        //Holds a jump amount that has been normalized over the number of jump frames
extern WORD jump_reduction;        //Holds the reduction amount that has been normalized over the number of jump frames
extern WORD boost_val;
extern UBYTE enemy_bounce;

//WALKING AND RUNNING VARIABLES
extern WORD pl_vel_x;              //Tracks the player's x-velocity between frames
extern WORD pl_vel_y;              //Tracks the player's y-velocity between frames

//VARIABLES FOR CAMERAS
extern WORD *edge_left;
extern WORD *edge_right;
extern WORD mod_image_right;
extern WORD mod_image_left;

extern UBYTE grounded;
extern UBYTE plat_drop_through;
extern BYTE run_stage;
extern BYTE last_wall;
extern BYTE col;