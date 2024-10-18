#ifndef ACTOR_BEHAVIOR_H
#define ACTOR_BEHAVIOR_H

#define BEHAVIOR_ACTIVATION_THRESHOLD 168
#define BEHAVIOR_DEACTIVATION_THRESHOLD 176
#define BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD -8

#include <gbdk/platform.h>
#include "actor.h"

void actor_behavior_init(void) BANKED;
void actor_behavior_update(void) BANKED;

extern UBYTE actor_behavior_ids[MAX_ACTORS];
extern UBYTE actor_states[MAX_ACTORS];
extern WORD actor_vel_x[MAX_ACTORS];
extern WORD actor_vel_y[MAX_ACTORS];
extern UBYTE actor_counter_a[MAX_ACTORS];
extern UBYTE actor_counter_b[MAX_ACTORS];
extern UBYTE actor_linked_actor_idx[MAX_ACTORS];

extern WORD current_actor_x;
extern WORD new_actor_x;
extern WORD new_actor_y;
extern WORD col_tx;
extern WORD col_ty;

#endif
