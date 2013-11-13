
/*
 * Pebble Stopwatch, Countdown, Yactimer - Model to allow multi-timers testimg etc.
 * Copyright (C) 2013  Mike Moore
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _MMYACHTIMERCONTROL_H_
#define _MMYACHTIMERCONTROL_H_
#include <pebble.h>
#include "yachtimermodel.h"


typedef enum  ButtonControl {
	BUTTON_LAP = BUTTON_ID_DOWN,
	BUTTON_RUN = BUTTON_ID_SELECT,
	BUTTON_RESET = BUTTON_ID_UP
} ButtonControlType;

typedef enum ButtonPress {
	BUTTON_SHORT = true,
	BUTTON_LONG = false
} ButtonPressType;

#define TIMER_UPDATE 180563
#define MODES 5 // Number of watch types stopwatch, coutdown, yachttimer, watch
#define TICKREMOVE 5
#define CNTDWNCFG 99
#define MAX_TIME  (ASECOND * 60 * 60 * 24)

// The documentation claims this is defined, but it is not.
// Define it here for now.
#ifndef APP_TIMER_INVALID_HANDLE
    #define APP_TIMER_INVALID_HANDLE NULL
#endif

typedef struct moderesource {
	// Mode either a model mode or config or an extension mode
        int mode;
	// This allows app to drive what is displayed
        int resourceid;
	// This gets loaded in init and release in deinit
        BitmapLayer *modeImage;
	GBitmap *imgBmp; 
        // if config what the adjustment amount is by
        int adjustnum;
} ModeResource; 

struct yachtTimerControl;

typedef void (*ClickExtensionHandler)(struct yachtTimerControl *controller, ClickRecognizerRef recognizer, void *context);


typedef struct controlExtension {
	ButtonControlType   buttonType;
	ButtonPressType     buttonPress;
	ClickExtensionHandler handler;
} YachtTimerControlInheritance;

typedef struct yachtTimerControlExtension {
	int length;
	YachtTimerControlInheritance *extension; 
} YachtTimerControlExtension;

typedef struct yachtTimerControl {
	// List of modes for controller to manage passed in init
	ModeResource *resources;
	// Number of modes
	int numModes;
	// Underlying model
	YachtTimer theModel;
	// Used to keep track of last time so handle_tick gets past right flags
	struct tm  theLastTime;
	// Passed at construction for final countdown vibe
	VibePattern *endVibePattern;
	// Ticks since last toggle
	int ticks;
	// handle to timer events set up and managed by controller
	AppTimer *update_timer; 
	// between loops remebers what last set to
	// Used by model to work out when in fine grained timing mode
	int ticklen;
	// Current mode of controller not mode of model
	int mode;
	// record mode on each start or reset. So if not in mode can get to what is needed.
	int startappmode;
	// Tick handler where display is meant to happen
	TickHandler tickHandler;
	// Click controls overrides if needed
	YachtTimerControlExtension *extension;
	bool autohidebitmaps;
	
} YachtTimerControl;
	

// Constructor
void yachtimercontrol_init(	YachtTimerControl *theControl,
				Window *window,
				ModeResource *modeResource, 
				int numModes, 
				GRect positionIcon,  
				TickHandler tickHandler);

// Destructor
void yachtimercontrol_deinit(YachtTimerControl *theControl);

// methods
// allow access to underlying model for view access
YachtTimer* yachtimercontrol_getModel(YachtTimerControl *theControl);

// Handlers for clicks
void yachtimercontrol_toggle_stopwatch_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context);
void yachtimercontrol_toggle_mode(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context);
void yachtimercontrol_reset_stopwatch_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context);
void yachtimercontrol_stop_stopwatch(YachtTimerControl *theControl);
void yachtimercontrol_start_stopwatch(YachtTimerControl *theControl);
void yachtimer_lap_time_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context);

// Boiler plat config handling
void yachtimercontrol_default_config_provider(void *context);
// Hook to ticks
void yachtimercontrol_handle_timer(void  *data);

// method that adjusts the config
void yachtimercontrol_config_watch(YachtTimerControl *theControl,int increment);
void yachtimercontrol_update_hand_positions(YachtTimerControl *theControl);

// Accessors and setters
YachtTimerControlExtension *yachtimercontrol_getExtension(YachtTimerControl *theControl);
void yachtimercontrol_setExtension(YachtTimerControl *theControl, YachtTimerControl *theExtension);
VibePattern *yachtimercontrol_getEndVibePattern(YachtTimerControl *theControl);
void yachtimercontrol_setEndVibePattern(YachtTimerControl *theControl, VibePattern *theVibePattern);
bool yachtimercontrol_getAutohide(YachtTimerControl *theControl);
void yachtimercontrol_setAutohide(YachtTimerControl *theControl,bool autoHide);

// Private method for controller ??

#endif
