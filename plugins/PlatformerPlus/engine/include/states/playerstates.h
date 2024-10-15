#include "states/platform.h"

void fall_state(void) BANKED;
void ground_state(void) BANKED;
void crouch_state(void) BANKED;
void jump_state(void) BANKED;
void climb_state(void) BANKED;
void blank_state(void) BANKED;
void swim_state(void) BANKED;

typedef enum {              //Datatype for tracking states
    FALL_INIT = 0,
    FALL_STATE = 1,
    FALL_END = 2,
    GROUND_INIT = 3,
    GROUND_STATE = 4,
    GROUND_END = 5,
	CROUCH_INIT = 6,
	CROUCH_STATE = 7,
	CROUCH_END = 8,
	SKID_INIT = 9,
	SKID_STATE = 10,
	SKID_END = 11,
    JUMP_INIT = 12,
    JUMP_STATE = 13,
    JUMP_END = 14,
    CLIMB_INIT = 15,
    CLIMB_STATE = 16,
    CLIMB_END = 17,
    BLANK_INIT = 18,
    BLANK_STATE = 19,
	BLANK_END = 20,
	SWIM_INIT = 21,
	SWIM_STATE = 22,
	SWIM_END = 23
}  pStates;

extern pStates plat_state;
extern pStates que_state;
extern script_state_t state_events[24];
       
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

extern UBYTE crouched;
extern UBYTE que_attacking;
extern UBYTE stat_attacking;