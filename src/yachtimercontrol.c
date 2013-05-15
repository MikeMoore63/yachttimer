
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

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "yachtimercontrol.h"

#define NUMDEFHANDLERS 4

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

static void handle_button(ClickRecognizerRef recognizer, Window *window, ButtonControlType buttonType, ButtonPressType pressType ) {
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
				userExtension->extension[i].handler(this,recognizer,window);
			}
		}
	}
}

static void button_lap_short_handler(ClickRecognizerRef recognizer, Window *window) {
	handle_button(recognizer,window, BUTTON_LAP, BUTTON_SHORT);	
}

static void button_lap_long_handler(ClickRecognizerRef recognizer, Window *window) {
	handle_button(recognizer,window, BUTTON_LAP, BUTTON_LONG);	
}

static void button_reset_short_handler(ClickRecognizerRef recognizer, Window *window) {
	handle_button(recognizer,window, BUTTON_RESET, BUTTON_SHORT);	
}
static void button_reset_long_handler(ClickRecognizerRef recognizer, Window *window) {
	handle_button(recognizer,window, BUTTON_RESET, BUTTON_LONG);	
}

static void button_run_short_handler(ClickRecognizerRef recognizer, Window *window) {
	handle_button(recognizer,window, BUTTON_RUN, BUTTON_SHORT);	
}
static void button_run_long_handler(ClickRecognizerRef recognizer, Window *window) {
	handle_button(recognizer,window, BUTTON_RUN, BUTTON_LONG);	
}
void yachtimercontrol_default_config_provider(ClickConfig **config, Window *window) {
    config[BUTTON_RUN]->click.handler = (ClickHandler)button_run_short_handler;
    config[BUTTON_RUN]->long_click.handler = (ClickHandler)button_run_long_handler;
    config[BUTTON_LAP]->click.handler = (ClickHandler) button_lap_short_handler;
    config[BUTTON_LAP]->long_click.handler = (ClickHandler) button_lap_long_handler;
    config[BUTTON_RESET]->click.handler = (ClickHandler)button_reset_short_handler;
    config[BUTTON_RESET]->long_click.handler = (ClickHandler)button_reset_long_handler;
}


void yachtimercontrol_init(	YachtTimerControl *theControl,
				AppContextRef *app, 
				Window *window, 
				ModeResource *modeResource, 
				int numModes, 
				GRect positionIcon,  
				PebbleAppTickHandler tickHandler )
{
	this = theControl;

	this->app = app;
	this->resources = modeResource;
	this->numModes = numModes;
	this->endVibePattern = &default_start_pattern;
	
	this->ticks = 0;
	this->update_timer = APP_TIMER_INVALID_HANDLE;
	this->ticklen = 0;
	this->tickHandler = tickHandler;

	// Set standard handler
	this->extension = &defaultExtension;


  	// Arrange for user input. Default 
  	window_set_click_config_provider(window, (ClickConfigProvider) yachtimercontrol_default_config_provider);


	for (int i=0;i<numModes;i++)
  	{
        	bmp_init_container(modeResource[i].resourceid,&(modeResource[i].modeImage));
       		layer_set_frame(&(modeResource[i].modeImage.layer.layer), positionIcon);
		layer_set_hidden(&(modeResource[i].modeImage.layer.layer), true);
        	layer_add_child(window_get_root_layer(window),&(modeResource[i].modeImage.layer.layer));
  	}		

	// initialise the modelad set to starting mode.
	// resourcelist passed in sets the starting mode.
	yachtimer_init(yachtimercontrol_getModel(this),(this->resources)[0].mode);
	yachtimer_setConfigTime(yachtimercontrol_getModel(this),ASECOND * 60 * 10);
	yachtimer_tick(yachtimercontrol_getModel(this),0);
	// mode of controller offset in resource array 
	this->mode = 0;
	// Starting mode of model
	this->startappmode = (this->resources)[0].mode;
	yachtimercontrol_stop_stopwatch(this);
	PblTm *tick_time;
	tick_time = yachtimer_getPblDisplayTime(yachtimercontrol_getModel(this));
	memcpy(&(this->theLastTime),tick_time,sizeof(PblTm));
	this->theLastTime.tm_yday = tick_time->tm_yday;
  	this->theLastTime.tm_mon = tick_time->tm_mon;
  	this->theLastTime.tm_year = tick_time->tm_year;
    this->autohidebitmaps=true;
	
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
	return(&(theControl->theModel));
}
void yachtimercontrol_deinit(YachtTimerControl *theControl)
{
	ModeResource *modeResource=theControl->resources;
	
	for (int i=0;i<theControl->numModes;i++)
        {
		bmp_deinit_container(&(modeResource[i].modeImage));
	}
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

void yachtimercontrol_toggle_mode(YachtTimerControl *theControl, ClickRecognizerRef recognizer, Window *window)
{
	YachtTimer *myYachtTimer = yachtimercontrol_getModel(theControl);
	ModeResource *theResources = theControl->resources;

         // Can only set to whatever number of modes are defined.
         theControl->mode++;
         theControl->mode  = (theControl->mode  == theControl->numModes) ?0:(theControl->mode);
         yachtimer_setMode(myYachtTimer,theControl->resources[theControl->mode].mode);

         for (int i=0;i<theControl->numModes;i++)
         {
               layer_set_hidden( &(theResources[i].modeImage.layer.layer), (theControl->mode == i)?false:true);
         }
         theControl->ticks = 0;

         yachtimercontrol_update_hand_positions(theControl);
}
void yachtimercontrol_toggle_stopwatch_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, Window *window) {
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
void yachtimercontrol_reset_stopwatch_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, Window *window) {
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
void yachtimer_lap_time_handler(YachtTimerControl *theControl, ClickRecognizerRef recognizer, Window *window) {
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
        yachtimercontrol_handle_timer(theControl->app,theControl->update_timer,TIMER_UPDATE);
}

// Wrap normal timer mode with stopwatch handling
void yachtimercontrol_handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie ) {
  (void)ctx;
  PebbleTickEvent theEvent;
    PblTm *theTime;
    PblTm myTime;
  YachtTimer *myYachtTimer = yachtimercontrol_getModel(this);

   if(cookie == TIMER_UPDATE)
   {
          yachtimer_tick(myYachtTimer,ASECOND);
          this->ticklen = yachtimer_getTick(myYachtTimer);
          theEvent.tick_time = yachtimer_getPblDisplayTime(myYachtTimer);
          theTime = yachtimer_getPblLastTime(myYachtTimer);
          memcpy(&myTime,theTime,sizeof(PblTm));
          theTime = &myTime;

          // Work out time changed
          // In all modes do hors minutes and seconds
          // In non-watch modes have day, date, day of week etc follow clock
          theEvent.units_changed = 0;
          theEvent.units_changed |= (this->theLastTime.tm_sec != theEvent.tick_time->tm_sec)?SECOND_UNIT:theEvent.units_changed;
          theEvent.units_changed |= (this->theLastTime.tm_min != theEvent.tick_time->tm_min)?MINUTE_UNIT:theEvent.units_changed;
          theEvent.units_changed |= (this->theLastTime.tm_hour != theEvent.tick_time->tm_hour)?HOUR_UNIT:theEvent.units_changed;
          if(yachtimer_getMode(myYachtTimer) == WATCHMODE)
          {
                  theEvent.units_changed |= (this->theLastTime.tm_yday != theEvent.tick_time->tm_yday)?DAY_UNIT:theEvent.units_changed;
                  theEvent.units_changed |= (this->theLastTime.tm_mon != theEvent.tick_time->tm_mon)?MONTH_UNIT:theEvent.units_changed;
                  theEvent.units_changed |= (this->theLastTime.tm_year != theEvent.tick_time->tm_year)?YEAR_UNIT:theEvent.units_changed;
           }
           else
           {

                  theEvent.units_changed |= (this->theLastTime.tm_yday != theTime->tm_yday)?DAY_UNIT:theEvent.units_changed;
                  theEvent.units_changed |= (this->theLastTime.tm_mon != theTime->tm_mon)?MONTH_UNIT:theEvent.units_changed;
                  theEvent.units_changed |= (this->theLastTime.tm_year != theTime->tm_year)?YEAR_UNIT:theEvent.units_changed;
           }
           memcpy(&(this->theLastTime),theEvent.tick_time,sizeof(PblTm));

          if(yachtimer_getMode(myYachtTimer) != WATCHMODE)
          {
              theEvent.tick_time->tm_yday = theTime->tm_yday;
              theEvent.tick_time->tm_mon = theTime->tm_mon;
              theEvent.tick_time->tm_year = theTime->tm_year;
              theEvent.tick_time->tm_wday = theTime->tm_wday;
              theEvent.tick_time->tm_mday = theTime->tm_mday;
          }
          if(this->ticks <= TICKREMOVE)
          {
                theEvent.units_changed = SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT;
          }

          // Emulate every second tick
          if(this->update_timer != APP_TIMER_INVALID_HANDLE) {
              if(app_timer_cancel_event(this->app, this->update_timer)) {
                  this->update_timer = APP_TIMER_INVALID_HANDLE;
              }
          }
          // All second only stopwatches need a second rather than using ticklen
          this->update_timer = app_timer_send_event(ctx, 1000, TIMER_UPDATE);
          this->ticks++;
          if(this->ticks == TICKREMOVE && this->autohidebitmaps)
          {
                ModeResource *theResources = this->resources;
                for(int i=0;i<this->numModes;i++)
                {
                    layer_set_hidden( &(theResources[i].modeImage.layer.layer), true);
                }
          }
          theTimeEventType event = yachtimer_triggerEvent(myYachtTimer);

        if(event == MinorTime) vibes_double_pulse();
        if(event == MajorTime) vibes_enqueue_custom_pattern(*(this->endVibePattern));
        this->tickHandler(this->app,&theEvent);
   }
}
