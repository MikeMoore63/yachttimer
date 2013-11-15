
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

#include <pebble.h>
#include "yachtimercontrol.h"

#define NUMDEFHANDLERS 4

struct YachtTimerControl {
        // List of modes for controller to manage passed in init
        ModeResource *resources;
        // Number of modes
        int numModes;
        // Underlying model
        YachtTimer *theModel;
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

} ;

// Singleton Yacht Control
// No context in cal backs so need this to route back to right calls
static YachtTimerControl *this;

static VibePattern default_start_pattern =  {
  .durations = (uint32_t []) {100, 300, 300, 300, 100, 300},
  .num_segments = 6
};

// Default button handlers
static YachtTimerControlInheritance defaultExtensionList[NUMDEFHANDLERS] = {
	{ BUTTON_RUN, BUTTON_SHORT, &yachtimercontrol_toggle_stopwatch_handler },
	{ BUTTON_LAP, BUTTON_SHORT, &yachtimercontrol_toggle_mode },
	{ BUTTON_RESET, BUTTON_SHORT, &yachtimercontrol_reset_stopwatch_handler },
    { BUTTON_LAP, BUTTON_LONG, &yachtimer_lap_time_handler }
};


// the default Extensions
static YachtTimerControlExtension defaultExtension = {NUMDEFHANDLERS,defaultExtensionList };

static void handle_button(ClickRecognizerRef recognizer, void *context, ButtonControlType buttonType, ButtonPressType pressType ) {
	YachtTimerControlExtension *userExtension = this->extension;

	// Note if controls mode is not one that is recognised these do nothing
	// Loop through extensions calling them

	// Loop through extension handlers calling them
	if(this->extension != NULL)
	{
		for(int i=0;i<userExtension->length;i++)
		{
			if(userExtension->extension[i].buttonType == buttonType && userExtension->extension[i].buttonPress == pressType)
			{
				userExtension->extension[i].handler(this,recognizer,context);
			}
		}
	}
}

static void button_lap_short_handler(ClickRecognizerRef recognizer, void *context) {
	handle_button(recognizer,context, BUTTON_LAP, BUTTON_SHORT);	
}

static void button_lap_long_handler(ClickRecognizerRef recognizer, void *context) {
	handle_button(recognizer,context, BUTTON_LAP, BUTTON_LONG);	
}

static void button_reset_short_handler(ClickRecognizerRef recognizer, void *context) {
	handle_button(recognizer,context, BUTTON_RESET, BUTTON_SHORT);	
}
static void button_reset_long_handler(ClickRecognizerRef recognizer, void *context) {
	handle_button(recognizer,context, BUTTON_RESET, BUTTON_LONG);	
}

static void button_run_short_handler(ClickRecognizerRef recognizer, void *context) {
	handle_button(recognizer,context, BUTTON_RUN, BUTTON_SHORT);	
}
static void button_run_long_handler(ClickRecognizerRef recognizer, void *context) {
	handle_button(recognizer,context, BUTTON_RUN, BUTTON_LONG);	
}
void yachtimercontrol_default_config_provider( void *context) {
    window_single_click_subscribe(BUTTON_RUN, button_run_short_handler);
    window_long_click_subscribe(BUTTON_RUN, 1000, button_run_long_handler,NULL);
    window_single_click_subscribe(BUTTON_LAP, button_lap_short_handler);
    window_long_click_subscribe(BUTTON_LAP, 1000, button_lap_long_handler,NULL);
    window_single_click_subscribe(BUTTON_RESET, button_reset_short_handler);
    window_long_click_subscribe(BUTTON_RESET, 1000, button_reset_long_handler,NULL);
    /*
    config[BUTTON_RUN]->click.handler = (ClickHandler)button_run_short_handler;
    config[BUTTON_RUN]->long_click.handler = (ClickHandler)button_run_long_handler;
    config[BUTTON_LAP]->click.handler = (ClickHandler) button_lap_short_handler;
    config[BUTTON_LAP]->long_click.handler = (ClickHandler) button_lap_long_handler;
    config[BUTTON_RESET]->click.handler = (ClickHandler)button_reset_short_handler;
    config[BUTTON_RESET]->long_click.handler = (ClickHandler)button_reset_long_handler;
    */
}


YachtTimerControl *yachtimercontrol_create(
				Window *window, 
				ModeResource *modeResource, 
				int numModes, 
				GRect positionIcon,  
				TickHandler tickHandler )
{
	YachtTimerControl *theControl = malloc(sizeof(YachtTimerControl));
	if(theControl)
	{
		this = theControl;

		this->resources = modeResource;
		this->numModes = numModes;
		this->endVibePattern = &default_start_pattern;
		
		this->ticks = 0;
		this->update_timer = NULL;
		this->ticklen = 0;
		this->tickHandler = tickHandler;

		// Set standard handler
		this->extension = &defaultExtension;


		// Arrange for user input. Default 
		window_set_click_config_provider(window, (ClickConfigProvider) yachtimercontrol_default_config_provider);


		for (int i=0;i<numModes;i++)
		{
			modeResource[i].imgBmp = gbitmap_create_with_resource(modeResource[i].resourceid);
			modeResource[i].modeImage = bitmap_layer_create(modeResource[i].imgBmp->bounds);
			bitmap_layer_set_bitmap(modeResource[i].modeImage,modeResource[i].imgBmp);
			// bmp_init_container(modeResource[i].resourceid,&(modeResource[i].modeImage));
			layer_set_frame((Layer *)modeResource[i].modeImage, positionIcon);
			layer_set_hidden((Layer *)modeResource[i].modeImage, true);
			layer_add_child(window_get_root_layer(window),(Layer *)modeResource[i].modeImage);
		}		

		// initialise the modelad set to starting mode.
		// resourcelist passed in sets the starting mode.
		this->theModel = yachtimer_create((this->resources)[0].mode);
		yachtimer_setConfigTime(yachtimercontrol_getModel(this),ASECOND * 60 * 10);
		yachtimer_tick(yachtimercontrol_getModel(this),0);
		// mode of controller offset in resource array 
		this->mode = 0;
		// Starting mode of model
		this->startappmode = (this->resources)[0].mode;
		yachtimercontrol_stop_stopwatch(this);
		struct tm  *tick_time;
		tick_time = yachtimer_getPblDisplayTime(yachtimercontrol_getModel(this));
		memcpy(&(this->theLastTime),tick_time,sizeof(struct tm));
		this->theLastTime.tm_yday = tick_time->tm_yday;
		this->theLastTime.tm_mon = tick_time->tm_mon;
		this->theLastTime.tm_year = tick_time->tm_year;
	        this->autohidebitmaps=true;
	}
	return(theControl);
}
bool yachtimercontrol_getAutohide(YachtTimerControl *theControl)
{
    return(theControl->autohidebitmaps);
}
void yachtimercontrol_setAutohide(YachtTimerControl *theControl, bool autoHide)
{
    theControl->autohidebitmaps=autoHide;
}
YachtTimer* yachtimercontrol_getModel(YachtTimerControl *theControl)
{
	return(theControl->theModel);
}
void yachtimercontrol_destroy(YachtTimerControl *theControl)
{
	ModeResource *modeResource=theControl->resources;
	
	for (int i=0;i<theControl->numModes;i++)
        {
		bitmap_layer_destroy(modeResource[i].modeImage);
		gbitmap_destroy(modeResource[i].imgBmp);
		// bmp_deinit_container(&(modeResource[i].modeImage));
	}
	yachtimer_destroy(yachtimercontrol_getModel(theControl));
	free(theControl);
}


void yachtimercontrol_config_watch(YachtTimerControl *theControl, int increment)
{
    int adjustnum = 0;
    YachtTimer *myYachtTimer = yachtimercontrol_getModel(theControl);;

    // even if running allow minute changes
    switch(theControl->resources[theControl->mode].mode)
        {
        // Ok so we want to lower countdown config
        // Down in increments of 1 minute
        case CNTDWNCFG:
                adjustnum=ASECOND * 60;
                theControl->ticks=0;
                break;

        }


        /* for non adjust appmodes does nothing as adjustnum is 0 */
        time_t new_time=0;

        /* if running adjust running time otherwise adjust config time */
        if(yachtimer_isRunning(myYachtTimer))
        {
                new_time =  yachtimer_getElapsed(myYachtTimer) + (increment * adjustnum );
                if(new_time > MAX_TIME) new_time = yachtimer_getElapsed(myYachtTimer);
                yachtimer_setElapsed(myYachtTimer, new_time);
        }
        else
        {
                new_time =  yachtimer_getConfigTime(myYachtTimer) + (increment * adjustnum );
                // Cannot sert below 0
                // Can set above max display time
                // so keep it displayable
                if(new_time > MAX_TIME) new_time = MAX_TIME;
                yachtimer_setConfigTime(myYachtTimer, new_time);

        }



}

void yachtimercontrol_start_stopwatch(YachtTimerControl *theControl)
{
	YachtTimer *myYachtTimer = yachtimercontrol_getModel(theControl);
 	yachtimer_start(myYachtTimer);

    	// default start mode
    	theControl->startappmode = yachtimer_getMode(myYachtTimer);;
    	yachtimercontrol_update_hand_positions(theControl);
}
void yachtimercontrol_stop_stopwatch(YachtTimerControl *theControl)
{
	YachtTimer *myYachtTimer = yachtimercontrol_getModel(theControl);
    	yachtimer_stop(myYachtTimer);
    	yachtimercontrol_update_hand_positions(theControl);
}

void yachtimercontrol_toggle_mode(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context)
{
	YachtTimer *myYachtTimer = yachtimercontrol_getModel(theControl);
	ModeResource *theResources = theControl->resources;

         // Can only set to whatever number of modes are defined.
         theControl->mode++;
         theControl->mode  = (theControl->mode  == theControl->numModes) ?0:(theControl->mode);
         yachtimer_setMode(myYachtTimer,theControl->resources[theControl->mode].mode);

         for (int i=0;i<theControl->numModes;i++)
         {
               layer_set_hidden( (Layer *)theResources[i].modeImage, (theControl->mode == i)?false:true);
         }
         theControl->ticks = 0;

         yachtimercontrol_update_hand_positions(theControl);
}
void yachtimercontrol_toggle_stopwatch_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context) {
    YachtTimer *myYachtTimer = yachtimercontrol_getModel(theControl);

    switch(theControl->resources[theControl->mode].mode)
    {
        case YACHTIMER:
        case STOPWATCH:
        case COUNTDOWN:
            if(yachtimer_isRunning(myYachtTimer)) {
                yachtimercontrol_stop_stopwatch(theControl);
            } else {
                yachtimercontrol_start_stopwatch(theControl);
            }
            break;
        default:
            yachtimercontrol_config_watch(theControl,-1);
    }
    yachtimercontrol_update_hand_positions(theControl);
}
void yachtimercontrol_reset_stopwatch_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context) {
    YachtTimer *myYachtTimer = yachtimercontrol_getModel(theControl);

    switch(theControl->resources[theControl->mode].mode)
    {
        case STOPWATCH:
        case YACHTIMER:
        case COUNTDOWN:
            yachtimer_reset(myYachtTimer);
            if(yachtimer_isRunning(myYachtTimer))
            {
                 yachtimercontrol_stop_stopwatch(theControl);
                 yachtimercontrol_start_stopwatch(theControl);
            }
            else
            {
                yachtimercontrol_stop_stopwatch(theControl);
            }

            break;
        default:
            ;
            // if not in config mode won't do anything which makes this easy
            yachtimercontrol_config_watch(theControl,1);
    }
    // Force redisplay
    yachtimercontrol_update_hand_positions(theControl);
}
void yachtimer_lap_time_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, void *context) {
    time_t t=0;
    
    // if not running will retunr 0 which is useless
    // so check timer is running before diaplaying stuff
    if(yachtimer_isRunning(yachtimercontrol_getModel(theControl)))
    {
        // returns laptime of current mode
        // if overrun timer willbe time since overrun started
        t=labs(yachtimer_getLap(yachtimercontrol_getModel(theControl)));
        switch(theControl->resources[this->mode].mode)
        {
            case STOPWATCH:
            case YACHTIMER:
            case COUNTDOWN:
                // save_lap_time(t);
                // have to use t
                t=labs(t);
                break;
        }
    }
    // Update countdown if desired
    // Note if running adjust elapsed if stopped config
    // does nothing if not in config mode.
    // yachtimercontrol_config_watch(theControl,-1);
    
    // Force redisplay
    yachtimercontrol_update_hand_positions(theControl);
}
void yachtimercontrol_update_hand_positions(YachtTimerControl *theControl)
{
	static uint32_t cookie = TIMER_UPDATE;
        yachtimercontrol_handle_timer(&cookie);
}

// Wrap normal timer mode with stopwatch handling
void yachtimercontrol_handle_timer(  void *data ) {
  TimeUnits units_changed;
  struct tm time_copy;
  struct tm  *tick_time;
  struct tm  *theTime;
  struct tm  myTime;
  YachtTimer *myYachtTimer = yachtimercontrol_getModel(this);
  uint32_t cookie = *(uint32_t *)data;

   if(cookie == TIMER_UPDATE)
   {
          yachtimer_tick(myYachtTimer,ASECOND);
          this->ticklen = yachtimer_getTick(myYachtTimer);
          tick_time = yachtimer_getPblDisplayTime(myYachtTimer);
	  memcpy(&time_copy,tick_time,sizeof(struct tm));
	  tick_time = &time_copy;
          theTime = yachtimer_getPblLastTime(myYachtTimer);
          memcpy(&myTime,theTime,sizeof(struct tm));
          theTime = &myTime;

          // Work out time changed
          // In all modes do hors minutes and seconds
          // In non-watch modes have day, date, day of week etc follow clock
          units_changed = 0;
          units_changed |= (this->theLastTime.tm_sec != tick_time->tm_sec)?SECOND_UNIT:units_changed;
          units_changed |= (this->theLastTime.tm_min != tick_time->tm_min)?MINUTE_UNIT:units_changed;
          units_changed |= (this->theLastTime.tm_hour != tick_time->tm_hour)?HOUR_UNIT:units_changed;
          if(yachtimer_getMode(myYachtTimer) == WATCHMODE)
          {
                  units_changed |= (this->theLastTime.tm_yday != tick_time->tm_yday)?DAY_UNIT:units_changed;
                  units_changed |= (this->theLastTime.tm_mon != tick_time->tm_mon)?MONTH_UNIT:units_changed;
                  units_changed |= (this->theLastTime.tm_year != tick_time->tm_year)?YEAR_UNIT:units_changed;
           }
           else
           {
                  units_changed |= (this->theLastTime.tm_yday != theTime->tm_yday)?DAY_UNIT:units_changed;
                  units_changed |= (this->theLastTime.tm_mon != theTime->tm_mon)?MONTH_UNIT:units_changed;
                  units_changed |= (this->theLastTime.tm_year != theTime->tm_year)?YEAR_UNIT:units_changed;
           }
           memcpy(&(this->theLastTime),tick_time,sizeof(struct tm));

          if(yachtimer_getMode(myYachtTimer) != WATCHMODE)
          {
              tick_time->tm_yday = theTime->tm_yday;
              tick_time->tm_mon = theTime->tm_mon;
              tick_time->tm_year = theTime->tm_year;
              tick_time->tm_wday = theTime->tm_wday;
              tick_time->tm_mday = theTime->tm_mday;
          }
          if(this->ticks <= TICKREMOVE)
          {
                units_changed = SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT;
          }

          // Emulate every second tick
          if(this->update_timer != NULL) {
              app_timer_cancel( this->update_timer);
              this->update_timer = NULL;
          }
          // All second only stopwatches need a second rather than using ticklen
          this->update_timer = app_timer_register( 1000, yachtimercontrol_handle_timer,data);
          this->ticks++;
          if(this->ticks == TICKREMOVE && this->autohidebitmaps)
          {
                ModeResource *theResources = this->resources;
                for(int i=0;i<this->numModes;i++)
                {
                    layer_set_hidden( (Layer *)theResources[i].modeImage, true);
                }
          }
          theTimeEventType event = yachtimer_triggerEvent(myYachtTimer);

        if(event == MinorTime) vibes_double_pulse();
        if(event == MajorTime) vibes_enqueue_custom_pattern(*(this->endVibePattern));
        this->tickHandler(tick_time, units_changed);
   }
}
