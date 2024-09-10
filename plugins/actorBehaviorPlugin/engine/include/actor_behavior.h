#ifndef ACTOR_BEHAVIOR_H
#define ACTOR_BEHAVIOR_H

#include <gbdk/platform.h>

void actor_behavior_init(void) BANKED;
void actor_behavior_update(void) BANKED;

typedef enum {              //Datatype for tracking actor Behaviors
	MOVE_NONE = 0,
	MOVE_H_C_G = 1, //Move left/right bouncing off collision and gravity
	MOVE_B_C_G = 2, //Bounces left/right bouncing off collision and gravity
	MOVE_H_C_G_A = 3, //Move left/right bouncing off collision and gravity avoiding edge
	MOVE_H_F = 4,  //Fly left/right ignoring collision and gravity
	MOVE_V_F = 5, // Fly up/down ignoring collision and gravity
	MOVE_V_L = 6, // Move up/down and loop back at bottom/top of screen once reaching top of screen
	MOVE_V_L_D = 7, // Move down and loop back at bottom of screen once reaching top of screen
	MOVE_BLOOPER = 8,
	MOVE_BULLETBILL = 9,
	MOVE_GREYCHEEP = 10,
	MOVE_REDCHEEP = 11,
	MOVE_PIRANHA = 12,
	MOVE_GPARATROOPAJ = 13,
	MOVE_GPARATROOPAF = 14,
	MOVE_RPARATROOPA = 15,
	MOVE_LAKITU = 16,
	MOVE_SPINY = 17,
	MOVE_FLYCHEEP = 18,
	MOVE_BOWSERFLAME = 19,
	MOVE_BOWSER = 20,
	MOVE_VINE = 21,
	MOVE_PLATCONTINUOUS = 22,
	MOVE_PLATDROP = 23,
	MOVE_PLATBALANCEL = 24,
	MOVE_PLATBALANCER = 25
}  aMoveBehaviors;

#endif
