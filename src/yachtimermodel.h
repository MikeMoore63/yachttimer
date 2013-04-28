
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

#ifndef _MMYACHTIMERMODEL_H_
#define _MMYACHTIMERMODEL_H_
#include "pebble_os.h"
/* #include "laps.h"
#include "common.h" */

//
// Stop watch timer based upon stop watch atimer code from Katherine Berry so many thanks to Katherine
// for a starting set of code.
//
// Aim of this is to have stop watch plus a countdown timer.
//
// Aim of countdown timer is to provide timier facilities for yacht racing
// Normally 5 mins for first gun, 4 minute gun 1 minute and then start.
// read more at link below.
//
// http://en.wikipedia.org/wiki/Racing_Rules_of_Sailing#Start_signal
//
// In countdown mode lap timer targets next rounded timer down or up. So if at anything below 5:00
// would reset to 4:00
//
// Anything below 4:00 and above 2:00 would be 4:00 if below 2:00 would be 1:00.
// Reset takes back to 5:00.
//
// Vibrates at 4 minutes and 1 minute and final timer with custome vibrate pattern.
// Times of reset are recorde as lap timers (Mainly as helps debug and for feedback as I use it).
//
// long press on select button toggles mode and aim is to toggle while running between modes
// as well as when stopped. Lap times/resets don't change.
// Will work on timer/chrono flag as well.
//
// to go in header to share with view
#define ASECOND 1000

// 5 minutes see rules of sailing
#define STARTGUNTIME 300 * ASECOND
#define BLUEPETERTIME 240 * ASECOND
#define ONEMINUTETIME 60  * ASECOND
#define SWITCH4OR1 120 * ASECOND
#define YACHTIMER       0
#define STOPWATCH       1
#define COUNTDOWN       2

// useful macros for leap year stuff
//
// http://www.codeproject.com/Articles/7358/Ultra-fast-Algorithms-for-Working-with-Leap-Years
// 
#define COUNT_LEAPS(Y)   ( ((Y)-1)/4 - ((Y)-1)/100 + ((Y)-1)/400 )
#define IS_LEAP_YEAR(Y)     ( ((Y)>0) && !((Y)%4) && ( ((Y)%100) || !((Y)%400) ) )
#define COUNT_DAYS(Y)  ( ((Y)-1)*365 + COUNT_LEAPS(Y) )
#define COUNT_YEARS(D)  ( 1 + ( (D) - COUNT_LEAPS((D)/365) )/365 )
#define YEAR_PLUS_DAYS(Y,D)  COUNT_YEARS(  D + COUNT_DAYS(Y) )
#define DAYS_BETWEEN_YEARS(A,B)  (COUNT_DAYS(B) - COUNT_DAYS(A))

typedef enum  TimeEventType {
        MinorTime,
        MajorTime,
        NoEvent
} theTimeEventType;

// Ok so plan is we model timers, stopwatches countdownas objects
// Plan is to allow as many as  needed and to keep start and stop and laps independant.
typedef struct yachtTimerModel {

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
        int last_lap_time /* = 0*/;
        int last_significant_time /* = 0*/;
	PblTm t;
	PblTm d;

} YachtTimer;

// Model methods
void yachtimer_init(YachtTimer *myTimer, int appmode);
void yachtimer_reset(YachtTimer *myTimer);
PblTm *yachtimer_getPblLastTime(YachtTimer *myTimer);
PblTm *yachtimer_getPblDisplayTime(YachtTimer *myTimer);
void yachtimer_stop(YachtTimer *myTimer);
void yachtimer_start(YachtTimer *myTimer);
bool yachtimer_isRunning(YachtTimer *myTimer);
int yachtimer_getMode(YachtTimer *myTimer);
time_t yachtimer_getLap(YachtTimer *myTimer);
void yachtimer_setMode(YachtTimer *myTimer,int appmode);
time_t yachtimer_getConfigTime(YachtTimer *myTimer);
void yachtimer_setConfigTime(YachtTimer *myTimer, time_t configTime);
time_t yachtimer_getElapsed(YachtTimer *myTimer);
void yachtimer_setElapsed(YachtTimer *myTimer, time_t elapsedTime);
// call timer returns reccomendation to next time relies on view to drive ticks
int yachtimer_getTick(YachtTimer *myTimer);
void yachtimer_tick(YachtTimer *myTimer,int ticklen);
time_t yachtimer_getDisplayTime(YachtTimer *myTimer);
theTimeEventType yachtimer_getEvent(YachtTimer *myTimer);
theTimeEventType yachtimer_triggerEvent(YachtTimer *myTimer);
bool yachtimer_countdownOverruning(YachtTimer *myTimer);

// A general method to convert times in seconds to PblTm 
// useful as then can use time format and even logic for watches
// To display stop watches.
void yachtimer_setPblTime(PblTm *pblTm,time_t toConvert);

#endif
