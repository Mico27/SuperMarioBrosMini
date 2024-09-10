#include "states/platform.h"

void start_level_init(void) BANKED;
void start_level_state(void) BANKED;
//void pipe_state(void) BANKED;
//void pole_state(void) BANKED;
//void dead_state(void) BANKED;
//void changesize_state(void) BANKED;
//void fireflower_state(void) BANKED;
//void end_level_state(void) BANKED;

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

//LEVEL START VARIABLES
extern UBYTE entry_mode; //0 = start of level, 1 = from pipe, 2 = from vine, 3 = from sky
extern UWORD entry_x; // entry's x position
extern UWORD entry_y; // entry's y position

