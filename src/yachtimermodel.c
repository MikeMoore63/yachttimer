
/*
 * Pebble Yachtimer model - Model implementation stopwatch, yachtimer, countdown
 * Copyright (C) 2013 Mike Moore
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

/* #include "laps.h"
#include "common.h" */
#include "yachtimermodel.h"

#define UNDEFINED_STORE_KEY 0xffff
#define STORAGE_VERSION 1   // To allow for incompatible stored structs can always add to end for extra but if need to change

// Ok so plan is we model timers, stopwatches countdownas objects
// Plan is to allow as many as  needed and to keep start and stop and laps independant.
// DON'T put pointers in HERE this is stored so neeeds to have all state with no references 
struct YachtTimer {
	uint32_t storageKey;
	uint32_t storageVersion;

        // Starts at 0 each tick adds to this time
        time_t elapsed_time /* = 0 */;

        //  flag for if start called or not
        bool started /* = false */ ;

        // We want hundredths of a second, but Pebble won't give us that.
        // Pebble's timers are also too inaccurate (we run fast for some reason)
        // Instead, we count our own time but also adjust ourselves every pebble
        // clock tick. We maintain our original offset in hundredths of a second
        // from the first tick. This should ensure that we always have accurate times.
        // Next two vars are all about this correction
        time_t start_time /* = 0 */;
        time_t last_pebble_time /* = 0 */;

        // Current timer countdown
        time_t countdown_time /* = STARTGUNTIME */;

        // Timer config time
        // Alter this when in config mode
        // When switch to timer mode set countdown to this time
        time_t config_time /* = STARTGUNTIME */;

        // 2 minutes focus constant if above go to 4 minutes if below go to 1 minute
        time_t switch_4or1_time /* = 120 * ASECOND */;
        // 1 minute
        // flags to track if we have buzzed for 4 and 1 min gun
        // as lap times reset will not buzz when button pressed but if not will buzz
        int appmode /* = YACHTIMER */;
        time_t last_lap_time /* = 0*/;
        time_t last_significant_time /* = 0*/;
        struct tm  t;
        struct tm  d;
} __attribute__((__packed__))  ;



// default days in month

// Set the time and milliseconds in the timer
static time_t get_pebble_time(YachtTimer *myTimer)
{
    // Milliseconds since January 1st 2012 in some timezone, 
    // Deciseconds since January 1st 2012 in some timezone, 
    // This does calc and sets member var
    // so if last time is needed in pebble format can be returned.
    struct tm  *t=&(myTimer->t);
    time_t gettime = time(NULL);
    struct tm *temp = localtime(&gettime);
    memcpy(t,temp,sizeof(struct tm));
    // get_time(t);
    
    time_t seconds = t->tm_sec;
    seconds += t->tm_min * 60;
    seconds += t->tm_hour * 3600;
    seconds += t->tm_yday * 86400;

    // Need to handle leap years
    seconds += DAYS_BETWEEN_YEARS(2012, t->tm_year ) * 86400;

    return seconds * ASECOND;
	
}
void yachtimer_setPblTime(struct tm  *pblTm,time_t displaytime)
{
	int daysinmonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	int i=0;
    time_t days=0,daysecs=0,daytotal=0;

	// converts display time to struct tm format
        // Allows pebble formatting of strings to be used for display
	// for elapsed though it is time since 1st Jan 1900
	//
	// http://pubs.opengroup.org/onlinepubs/7908799/xsh/time.h.html
	//

	// Calc days just once
	days = displaytime / 86400L;

	//  calc time in the day
	daysecs = displaytime - (days * 86400L);
	pblTm->tm_hour = (int)(daysecs / (60L * 60L));
	pblTm->tm_min = (int)((daysecs - (pblTm->tm_hour * (60L * 60L))) / 60L);
	pblTm->tm_sec = (int)(daysecs - (pblTm->tm_hour * (60L * 60L)) - (pblTm->tm_min * 60L)) ;
	
	// always never daylight saving for this
	pblTm->tm_isdst = false;

	// Use days plus 1900 to calculate year taking into account leap years
	pblTm->tm_year = (int)(YEAR_PLUS_DAYS(1900L,days));

	// Use the days between years to calculate day of year
	pblTm->tm_yday = (int)(days - DAYS_BETWEEN_YEARS(1900L,pblTm->tm_year));
	
	// need to have correct days in months to work out month
	if(IS_LEAP_YEAR(pblTm->tm_year)) daysinmonth[1]=29;
	else daysinmonth[1]=28;

	// Now find month and day of month
	for(i=0,daytotal=0;i<12;i++)
	{
		// if we have got this far month must be set.
		pblTm->tm_mon=i;
		pblTm->tm_mday = (int)(pblTm->tm_yday - daytotal);
		daytotal+=daysinmonth[i];

		// if we have reached end we are done
		if(daytotal >= pblTm->tm_yday)
			break;
	}

	// Now for the day of the week. Simply mod  of days
	// challenge as we don't have start time
	// jan 1st 1900 was a Monday but assuming will use day of week in display
	pblTm->tm_wday = days  % 7;
} 

struct tm  *yachtimer_getPblLastTime(YachtTimer *myTimer)
{
	return &(myTimer->t);
}

struct tm  *yachtimer_getPblDisplayTime(YachtTimer *myTimer)
{
	if(yachtimer_getMode(myTimer) == WATCHMODE)
	{
		return &(myTimer->t);
	}
	else
	{
		time_t displaytime = labs(yachtimer_getDisplayTime(myTimer)) / ASECOND;
		yachtimer_setPblTime(&(myTimer->d),displaytime);
	}

	return &(myTimer->d);
}

// Is the timer running i.e. started has been called
bool yachtimer_isRunning(YachtTimer *myTimer)
{
        return(myTimer->started);
}

// The timer can have many modes that can be dynamically set
// what is current mode
int yachtimer_getMode(YachtTimer *myTimer)
{
        return(myTimer->appmode);
}

// change the current mode.
void yachtimer_setMode(YachtTimer *myTimer,int appmode)
{
	// enters in a mode aim is to switch to another
        switch(appmode)
        {
                case STOPWATCH:
			// This is always Ok
                        myTimer->appmode = STOPWATCH;
                        break;
                case YACHTIMER:
			
                        myTimer->appmode = YACHTIMER;
                        // so change reset time
                        myTimer->countdown_time=STARTGUNTIME;
                        break;
                case COUNTDOWN:
                        myTimer->appmode = COUNTDOWN;
                        myTimer->countdown_time=myTimer->config_time;
                        break;
		case WATCHMODE:
			myTimer->appmode = WATCHMODE;
			break;
        }

}

// method to call in timer handler to push timer on.
// will allow less than a second if ticklen tells timer what it is
// note ticklen is ignored if drifts elapsed from pebble clock
void yachtimer_tick(YachtTimer *myTimer,int ticklen) {

        time_t pebble_time = get_pebble_time(myTimer);

        if(yachtimer_isRunning(myTimer)) 
	{
		if(!myTimer->last_pebble_time) myTimer->last_pebble_time = pebble_time;

		// Adding decisecond but base I want to change from millis to decis
		// if ASECOND = 1000 add ticklen / 1 = ticklen / (1000/1000)
                // if ASECOND = 100 add ticklen / 10 = ticklen / (1000/100)
		// if ASECOND = 10 add ticklen /100  = ticklen / (1000/10)
		// if ASECOND = 1 add ticklen /1000  = ticklen / (1000/1)
                // Not something I am planning but good if it works
                // if ASECOND = 5 add ticklen / 200  = ticklen / (1000/5)
		myTimer->elapsed_time += (ticklen / (1000/ASECOND));
		if(pebble_time > myTimer->last_pebble_time) 
                {
			// If it's the first tick, instead of changing our time we calculate the correct time.
			if(!myTimer->start_time) 
			{
			    myTimer->start_time = pebble_time - myTimer->elapsed_time;
			} else 
			{
			    myTimer->elapsed_time = pebble_time - myTimer->start_time;
			}
		}
        	myTimer->last_pebble_time = pebble_time;
	}
}
bool yachtimer_countdownOverruning(YachtTimer *myTimer)
{
	return (myTimer->elapsed_time >= myTimer->countdown_time);
}
// find out if a significant event
theTimeEventType yachtimer_getEvent(YachtTimer *myTimer)
{
        theTimeEventType retType = NoEvent;
        time_t displaytime = yachtimer_getDisplayTime(myTimer);
	// if greater than an hour minor event is hours
        int moddivider = ASECOND * 60 * 60;

        // if elapsed amd last significant must have been triggered already
	// don't repeat an event
        if(myTimer->last_significant_time < myTimer->elapsed_time )
        {
                int appmode = yachtimer_getMode(myTimer);
                switch(appmode)
                {
                        case YACHTIMER:
				// So not overruning
				if(displaytime >= 0)
				{
					if(displaytime <= BLUEPETERTIME)
					{
						if(myTimer->last_significant_time < (STARTGUNTIME - BLUEPETERTIME))
							retType = MinorTime;
					}
					if(displaytime <= ONEMINUTETIME)
					{
						if(myTimer->last_significant_time < (STARTGUNTIME - ONEMINUTETIME))
							retType = MinorTime;
					}
				}
                                break;
                        case COUNTDOWN:
				// can continue after overrun
				if(displaytime >= 0)
				{
					// if greater than an hour minor event is hours
					// if less than an hour  every 10 minutes is an event
					if(displaytime < (ASECOND * 60 * 60))  moddivider = ASECOND * 60 * 10;
					// if less than 10 minutes ever 5 minutes is an event
					if(displaytime < (ASECOND * 60 * 10))  moddivider = ASECOND * 60 * 5;
					// if less than 5 minutes  then every minute is an event
					if(displaytime < (ASECOND * 60 * 5))  moddivider = ASECOND * 60 ;
					// if less  than 1  minute every 30 seconds is an event
					if(displaytime < (ASECOND * 60 ))  moddivider = ASECOND * 30 ;
					// if less  than 1  minute every 30 seconds is an event
					if(displaytime < (ASECOND * 30 ))  moddivider = ASECOND * 10 ;
					// if less  than 30 seconds is an event
					if(displaytime < (ASECOND * 15 ))  moddivider = ASECOND * 5 ;

					// as we are using mod need to watch precision
					// could be being called at a coarse timing
					// display time is trying to be accurate to milliseconds
					// given timers not very accurate and if asked
					// we say 100 milli or second for wake up
					// so lets assume called once a second
					moddivider = moddivider / ASECOND;
					displaytime = displaytime / ASECOND;
				
					// now wehn we do this more likely to find significant event	
					if( !(displaytime % moddivider)) retType = MinorTime;
				}

                }
		//  if gone to end of timer will do negative time since timer end
		// but only do once i.e. if last event was less than countdown its major
		// Once beyond countdown no more majors
		// if toggled in yacht to countdown an dcountdown is longer will alert again
		// when time met if toggled back won't
                if(displaytime <= 0 && (myTimer->last_significant_time < myTimer->countdown_time ))
                {
                        retType = MajorTime;
                }


        }
        return(retType);
}
// set last significant stops get event returning event again
theTimeEventType yachtimer_triggerEvent(YachtTimer *myTimer)
{
        theTimeEventType retType = yachtimer_getEvent(myTimer);

	// set last event a second in the future to avoid double notification
        if(retType != NoEvent) myTimer->last_significant_time = myTimer->elapsed_time + ASECOND;
        return(retType);
}

time_t yachtimer_getDisplayTime(YachtTimer *myTimer)
{
        time_t display_time = myTimer->elapsed_time;
        if(myTimer->appmode != STOPWATCH)
        {
                // Some form of countdown still do logic
                // So even config is a countdown as order of modes is releavnt last mode seen by user was countdown
                display_time = myTimer->countdown_time - myTimer->elapsed_time;

        }
        return(display_time);
}
time_t yachtimer_getLap(YachtTimer *myTimer)
{
        time_t t = 0;
        switch(yachtimer_getMode(myTimer))
        {
                case STOPWATCH:
                        t = myTimer->elapsed_time - myTimer->last_lap_time;
                        myTimer->last_lap_time = myTimer->elapsed_time;
                        break;
                case YACHTIMER:
                case COUNTDOWN:
                        // Save when button was pressed for laptime display
                        t = myTimer->countdown_time - myTimer->elapsed_time;

			// Do this os when switching mode does what you expect
                        myTimer->last_lap_time = myTimer->elapsed_time;

                        if(yachtimer_getMode(myTimer)==YACHTIMER)
                        {

                                // Now target new gun as started if above 2 mins target 4
                                // do this even if halted
                                if(t >= SWITCH4OR1)
                                {
                                        yachtimer_setElapsed(myTimer, STARTGUNTIME - BLUEPETERTIME);
                                }
                                else  // otherwise target 1 minute
                                {
                                        yachtimer_setElapsed(myTimer, STARTGUNTIME - ONEMINUTETIME);
                                }
                        }
                        break;
            }
        return(t);
}
void yachtimer_reset(YachtTimer *myTimer)
{

    //  if reset what to  reset too
    // moved this as now have watch mode and reset won't do anything in watch mode
    // instead allow inverting is the plan
    // if not running it is reset anyhow so do nothing
    // if running set it to no time passed
    yachtimer_setElapsed(myTimer,0);
    
}
YachtTimer  *yachtimer_create_private( int appmode)
{
	YachtTimer *myTimer = malloc(sizeof(YachtTimer));
        if(myTimer)
	{
		myTimer->elapsed_time  = 0 ;
		myTimer->started = false  ;
		myTimer->start_time  = 0 ;
		myTimer->last_pebble_time  = 0 ;
		myTimer->countdown_time  = STARTGUNTIME ;
		myTimer->config_time  = ASECOND * 60 * 10 ;
		myTimer->appmode = appmode;
		myTimer->last_lap_time = 0;
		myTimer->last_significant_time = 0;
		myTimer->storageKey = UNDEFINED_STORE_KEY;
		myTimer->storageVersion = STORAGE_VERSION;
	}
	return(myTimer);
}
YachtTimer *yachtimer_create(int appmode)
{
	return(yachtimer_create_with_storage(DEFAULTYMSTORAGEKEY,appmode));
}
YachtTimer *yachtimer_create_with_storage(const uint32_t  storageKey, int defappmode)
{
	YachtTimer *myTimer = yachtimer_create_private(defappmode);
	if(myTimer)
	{
		if(persist_exists(storageKey))
		{
			persist_read_data(storageKey,(void *)myTimer,sizeof(YachtTimer));
		}
		// if struct has changed in non backward compatible way revert to default modes.
		if(myTimer->storageVersion != STORAGE_VERSION)
		{
			// note relies on no sub-allocs
			yachtimer_destroy(myTimer);
			myTimer = yachtimer_create_private(defappmode);
		}
		myTimer->storageKey = storageKey;
	}
	return(myTimer);
}
bool yachtimer_write_to_storage(YachtTimer *myTimer)
{
	size_t written=0;
	if(myTimer->storageKey != UNDEFINED_STORE_KEY)
	{
		written = persist_write_data(myTimer->storageKey,(void *)myTimer,sizeof(YachtTimer));
	}
	if(written == sizeof(YachtTimer))
	{
		return true;
	}
	return false;
}

void yachtimer_destroy(YachtTimer *myTimer)
{
	yachtimer_write_to_storage(myTimer);
	free(myTimer);
}
// Note idempotent
void yachtimer_stop(YachtTimer *myTimer)
{
        myTimer->started = false;
}
// Sadly not idempotent start_time reset need to check this is Ok
void yachtimer_start(YachtTimer *myTimer)
{
    myTimer->started = true;
    myTimer->last_pebble_time = 0;
    myTimer->start_time = 0;
}
bool yachtimer_isrunning(YachtTimer *myTimer)
{
        return myTimer->started;
}
// Asking for milliseconds for timers
int yachtimer_getTick(YachtTimer *myTimer)
{
        // default to a second
        int tick = 1000;

        // if below an hour recomend deciseconds
        if(myTimer->started && myTimer->elapsed_time <= (60 * 60 *  ASECOND) )
                tick = 100;

        return(tick);
}
int yachtimer_getmode(YachtTimer *myTimer)
{
        return myTimer->appmode;
}
time_t yachtimer_getConfigTime(YachtTimer *myTimer)
{
        return myTimer->config_time;
}

void yachtimer_setConfigTime(YachtTimer *myTimer, time_t configTime)
{
        // No point of 0 or lower time
        if(configTime > 0)
                myTimer->config_time=configTime;

	switch(yachtimer_getMode(myTimer))
	{
		case COUNTDOWN:
        		myTimer->countdown_time=myTimer->config_time;
	}
        myTimer->last_significant_time = 0;
}
time_t yachtimer_getElapsed(YachtTimer *myTimer)
{
        return myTimer->elapsed_time;
}

void yachtimer_setElapsed(YachtTimer *myTimer, time_t elapsedTime)
{
        // No point of 0 or lower time
        if(elapsedTime >= 0)
        {

		// do whats needed does a reset if  0 so that works if running or not
		// setting to non zero when not running would be wrong as last_pebble_time
		// will be zero making start_time incorrect.
		// As this is how stopwatch sync to clock
		if(yachtimer_isRunning(myTimer) || elapsedTime==0)
		{
			myTimer->elapsed_time=elapsedTime;
			myTimer->last_significant_time = myTimer->elapsed_time;
			myTimer->start_time = myTimer->last_pebble_time - myTimer->elapsed_time;
        		myTimer->last_lap_time = myTimer->elapsed_time;
		}
        }

}


