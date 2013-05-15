//
//  pebbleme.h
//  pebbleemulator
//
//  Created by Mike Moore on 12/05/2013.
//  Copyright (c) 2013 Mike Moore. All rights reserved.
//

#ifndef pebbleemulator_pebbleme_h
#define pebbleemulator_pebbleme_h

#define MAXTIMERS 10

typedef struct timeEvent {
	bool active; // True if an active timer
	AppTimerHandle handle;  // File Descriptor of timer
	uint32_t millis;  // time set of timer
	uint32_t cookie; // state to pass to call back
} TimeEvent;

typedef struct GContext
{
    GColor stroke_color;
    GColor fill_color;
    GColor text_color;
    GCompOp compositing_mode;
} GContextType;

// Guess at members based on methods
typedef struct appContext
{
	int activeTimers;
	// simply cap number of timers
	TimeEvent timeEvents[MAXTIMERS];
	Window *topWindow;
    GContextType theContext;
} AppContext;

void appcontext_init( AppContext *app_ctx);
void pbl_main(void *params);


#endif
