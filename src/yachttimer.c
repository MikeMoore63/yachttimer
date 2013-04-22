/*
 * Pebble Stopwatch - the big, ugly file.
 * Copyright (C) 2013 Katharine Berry
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

#include "laps.h"
#include "common.h"
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



#define MY_UUID { 0xE7, 0xB1, 0xD0, 0x1B, 0x7F, 0x1C, 0x48, 0x6A, 0x85, 0xB0, 0xDC, 0xA3, 0xD7, 0x4E, 0x7D, 0xCA }

PBL_APP_INFO(MY_UUID,
             "YachtTimer", "Mike Moore",
             4, 5, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);

static Window window;
static AppContextRef app;

// Main display
static TextLayer big_time_layer;
static TextLayer watch_layer_date;
static TextLayer watch_layer_timebig;
static TextLayer watch_layer_ampm;
static TextLayer seconds_time_layer;
static Layer line_layer;


// Lap time display
#define LAP_TIME_SIZE 5
static char lap_times[LAP_TIME_SIZE][11] = {"00:00:00.0", "00:01:00.0", "00:02:00.0", "00:03:00.0", "00:04:00.0"};
static TextLayer lap_layers[LAP_TIME_SIZE]; // an extra temporary layer
static int next_lap_layer = 0;
static int last_lap_time = 0;

// The documentation claims this is defined, but it is not.
// Define it here for now.
#ifndef APP_TIMER_INVALID_HANDLE
    #define APP_TIMER_INVALID_HANDLE 0xDEADBEEF
#endif

// Actually keeping track of time
static time_t elapsed_time = 0;
static bool started = false;
static AppTimerHandle update_timer = APP_TIMER_INVALID_HANDLE;
// We want hundredths of a second, but Pebble won't give us that.
// Pebble's timers are also too inaccurate (we run fast for some reason)
// Instead, we count our own time but also adjust ourselves every pebble
// clock tick. We maintain our original offset in hundredths of a second
// from the first tick. This should ensure that we always have accurate times.
static time_t start_time = 0;
static time_t last_pebble_time = 0;

#define STARTGUNTIME 300000

// Hard coded 5 minutes 
static time_t start_gun_time = STARTGUNTIME;

// Current timer countdown
static time_t countdown_time = STARTGUNTIME;

// Timer config time
// Alter this when in config mode
// When switch to timer mode set countdown to this time
static time_t config_time = STARTGUNTIME;

// 4 minutes
static time_t blue_peter_time = 240000;
// 2 minutes focus constant if above go to 4 minutes if below go to 1 minute
static time_t switch_4or1_time = 120000;
// 1 minute
static time_t one_minute_time = 60000;
static time_t one_second_time = 1000;
// flags to track if we have buzzed for 4 and 1 min gun
// as lap times reset will not buzz when button pressed but if not will buzz
static bool buzzatbluepeter=true, buzzatone=true;

#define	YACHTIMER	0
#define	STOPWATCH	1
#define COUNTDOWN	2
#define CNTDWNCNFGHRS	3
#define CNTDWNCNFGMIN	4
#define CNTDWNCNFGSEC	5
#define WATCH		6
#define MAXMODE		7
#define INVOFFSET	MAXMODE

// Define starting colour mode.
#define WHITE_ON_BLACK

#ifdef WHITE_ON_BLACK
static bool white_on_black=true;
static GColor foreground_colour = GColorWhite;
static GColor background_colour = GColorBlack;
// offset pointer used with app mode to work out which bit map to show.
static int inv_offset = 0;
#else
static bool white_on_black=false;
static GColor foreground_colour = GColorBlack;
static GColor background_colour = GColorWhite;
// offset pointer used with app mode to work out which bit map to show.
static int inv_offset = INVOFFSET;
#endif


// Have inverted as second set of bitmaps
// so use max multiplier
static int buttonmodeimages[MAXMODE * 2];
static BmpContainer button_labels[MAXMODE * 2];

// 
// Keep Katherine stopwatch mode
static int appmode = YACHTIMER;

// Custom vibration pattern
const VibePattern start_pattern = {
  .durations = (uint32_t []) {100, 300, 300, 300, 100, 300},
  .num_segments = 6
};

// Global animation lock. As long as we only try doing things while
// this is zero, we shouldn't crash the watch.
static int busy_animating = 0;
static GFont big_font, seconds_font, laps_font;

#define TIMER_UPDATE 1
#define FONT_BIG_TIME RESOURCE_ID_FONT_DEJAVU_SANS_BOLD_SUBSET_30
#define FONT_SECONDS RESOURCE_ID_FONT_DEJAVU_SANS_SUBSET_18
#define FONT_LAPS RESOURCE_ID_FONT_DEJAVU_SANS_SUBSET_22

#define BUTTON_LAP BUTTON_ID_DOWN
#define BUTTON_RUN BUTTON_ID_SELECT
#define BUTTON_RESET BUTTON_ID_UP

void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void countdown_config_handler(ClickRecognizerRef recognizer, Window *window);
void config_provider(ClickConfig **config, Window *window);
void handle_init(AppContextRef ctx);
time_t time_seconds();
void show_buttons();
void set_layer_colours();
void stop_stopwatch();
void start_stopwatch();
void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void toggle_mode(ClickRecognizerRef recognizer, Window *window);
void reset_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void update_stopwatch();
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie);
void pbl_main(void *params);
void draw_line(Layer *me, GContext* ctx);
void save_lap_time(time_t seconds);
void lap_time_handler(ClickRecognizerRef recognizer, Window *window);
void shift_lap_layer(PropertyAnimation* animation, Layer* layer, GRect* target, int distance_multiplier);
void config_watch(int appmode,int increment);

void handle_init(AppContextRef ctx) {
    app = ctx;

    window_init(&window, "Stopwatch");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, background_colour);
    window_set_fullscreen(&window, false);

    resource_init_current_app(&APP_RESOURCES);

    // Arrange for user input.
    window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);

    // Get our fonts
    big_font = fonts_load_custom_font(resource_get_handle(FONT_BIG_TIME));
    seconds_font = fonts_load_custom_font(resource_get_handle(FONT_SECONDS));
    laps_font = fonts_load_custom_font(resource_get_handle(FONT_LAPS));

    // Root layer
    Layer *root_layer = window_get_root_layer(&window);

    // Set up the big timer.
    text_layer_init(&big_time_layer, GRect(0, 5, 96, 35));
    text_layer_set_background_color(&big_time_layer, background_colour);
    text_layer_set_font(&big_time_layer, big_font);
    text_layer_set_text_color(&big_time_layer, foreground_colour);

    
    // in init only have either stopwatch as count up  or other modes count downs
    // if countdown then default to 5 mins same as yacht timer anyhow
    if(appmode == STOPWATCH)
    {
    	text_layer_set_text(&big_time_layer, "00:00");
    }
    else
    {
    	text_layer_set_text(&big_time_layer, "05:00");
    }
    text_layer_set_text_alignment(&big_time_layer, GTextAlignmentRight);
    layer_add_child(root_layer, &big_time_layer.layer);

    text_layer_init(&seconds_time_layer, GRect(96, 17, 49, 35));
    text_layer_set_background_color(&seconds_time_layer, background_colour);
    text_layer_set_font(&seconds_time_layer, seconds_font);
    text_layer_set_text_color(&seconds_time_layer, foreground_colour);
    text_layer_set_text(&seconds_time_layer, ".0");
    layer_add_child(root_layer, &seconds_time_layer.layer);

    // Set up the watch layer but hide it.
    text_layer_init(&watch_layer_date, GRect(0, 12, 96+49, 35));
    text_layer_set_background_color(&watch_layer_date, background_colour);
    text_layer_set_font(&watch_layer_date, laps_font);
    text_layer_set_text_color(&watch_layer_date, foreground_colour);

    // ensure full size layer with 12 hour format.
    text_layer_set_text(&watch_layer_date, "September 31");
    text_layer_set_text_alignment(&watch_layer_date, GTextAlignmentLeft);

    // by default hidden switch with big and little in watch mode
    layer_set_hidden(&watch_layer_date.layer,true);
    layer_add_child(root_layer, &watch_layer_date.layer);

    // Set up the watch layer but hide it.
    text_layer_init(&watch_layer_timebig, GRect(0, 52, 96, 35));
    text_layer_set_background_color(&watch_layer_timebig, background_colour);
    text_layer_set_font(&watch_layer_timebig, big_font);
    text_layer_set_text_color(&watch_layer_timebig, foreground_colour);

    // ensure full size layer with 12 hour format.
    text_layer_set_text(&watch_layer_timebig, "00:00");
    text_layer_set_text_alignment(&watch_layer_timebig, GTextAlignmentLeft);

    // by default hidden switch with big and little in watch mode
    layer_set_hidden(&watch_layer_timebig.layer,true);
    layer_add_child(root_layer, &watch_layer_timebig.layer);

    // Set up the watch layer but hide it. 
    text_layer_init(&watch_layer_ampm, GRect(96, 60, 49, 35));
    text_layer_set_background_color(&watch_layer_ampm, background_colour);
    text_layer_set_font(&watch_layer_ampm, laps_font);
    text_layer_set_text_color(&watch_layer_ampm, foreground_colour);

    // ensure full size layer with  am/pm
    text_layer_set_text(&watch_layer_ampm, "AM");
    text_layer_set_text_alignment(&watch_layer_ampm, GTextAlignmentLeft);

    // by default hidden switch with big and little in watch mode
    layer_set_hidden(&watch_layer_ampm.layer,true);
    layer_add_child(root_layer, &watch_layer_ampm.layer);

    // Draw our nice line.
    layer_init(&line_layer, GRect(0, 45, 144, 2));
    line_layer.update_proc = &draw_line;
    layer_add_child(root_layer, &line_layer);

    // Set up the lap time layers. These will be made visible later.
    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
        text_layer_init(&lap_layers[i], GRect(-139, 52, 139, 30));
        text_layer_set_background_color(&lap_layers[i], GColorClear);
        text_layer_set_font(&lap_layers[i], laps_font);
        text_layer_set_text_color(&lap_layers[i], foreground_colour);
        text_layer_set_text(&lap_layers[i], lap_times[i]);
        layer_add_child(root_layer, &lap_layers[i].layer);
    }

    // Add some button labels
    buttonmodeimages[STOPWATCH] = RESOURCE_ID_IMAGE_STOPWATCH_BUTTON_LABELS; 
    buttonmodeimages[YACHTIMER] = RESOURCE_ID_IMAGE_YACHTIMER_BUTTON_LABELS; 
    buttonmodeimages[COUNTDOWN] = RESOURCE_ID_IMAGE_COUNTDOWN_BUTTON_LABELS; 
    buttonmodeimages[CNTDWNCNFGHRS] = RESOURCE_ID_IMAGE_CNTDWNCNFGHRS_BUTTON_LABELS; 
    buttonmodeimages[CNTDWNCNFGMIN] = RESOURCE_ID_IMAGE_CNTDWNCNFGMIN_BUTTON_LABELS; 
    buttonmodeimages[CNTDWNCNFGSEC] = RESOURCE_ID_IMAGE_CNTDWNCNFGSEC_BUTTON_LABELS; 
    buttonmodeimages[WATCH] = RESOURCE_ID_IMAGE_WATCH_BUTTON_LABELS; 
    buttonmodeimages[STOPWATCH + INVOFFSET] = RESOURCE_ID_IMAGE_STOPWATCH_BUTTON_LABELSIV; 
    buttonmodeimages[YACHTIMER + INVOFFSET] = RESOURCE_ID_IMAGE_YACHTIMER_BUTTON_LABELSIV; 
    buttonmodeimages[COUNTDOWN + INVOFFSET] = RESOURCE_ID_IMAGE_COUNTDOWN_BUTTON_LABELSIV; 
    buttonmodeimages[CNTDWNCNFGHRS + INVOFFSET] = RESOURCE_ID_IMAGE_CNTDWNCNFGHRS_BUTTON_LABELSIV; 
    buttonmodeimages[CNTDWNCNFGMIN + INVOFFSET] = RESOURCE_ID_IMAGE_CNTDWNCNFGMIN_BUTTON_LABELSIV; 
    buttonmodeimages[CNTDWNCNFGSEC + INVOFFSET] = RESOURCE_ID_IMAGE_CNTDWNCNFGSEC_BUTTON_LABELSIV; 
    buttonmodeimages[WATCH + INVOFFSET] = RESOURCE_ID_IMAGE_WATCH_BUTTON_LABELSIV; 

    // Set up button layers normal and inverse
    for(int i=0;i<(MAXMODE * 2) ;i++) {
    	bmp_init_container(buttonmodeimages[i], &button_labels[i]);
    	layer_set_frame(&button_labels[i].layer.layer, GRect(130, 10, 14, 136));

	// Make sure active mode button only visible
	layer_set_hidden( &button_labels[i].layer.layer, ((appmode+inv_offset)==i?false:true));

	// add child layers to root_layer
    	layer_add_child(root_layer, &button_labels[i].layer.layer);
    }
    


    // Set up lap time stuff, too.
    init_lap_window();
 
    // Get timer going same as stoping stop watch.
    stop_stopwatch();
}

void handle_deinit(AppContextRef ctx) {
    for(int i=0;i<(MAXMODE*2);i++)
    	bmp_deinit_container(&button_labels[i]);
    fonts_unload_custom_font(big_font);
    fonts_unload_custom_font(seconds_font);
    fonts_unload_custom_font(laps_font);
}

void draw_line(Layer *me, GContext* ctx) {
    graphics_context_set_stroke_color(ctx, foreground_colour);
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(140, 0));
    graphics_draw_line(ctx, GPoint(0, 1), GPoint(140, 1));
}

void stop_stopwatch() {
    started = false;
    if(update_timer != APP_TIMER_INVALID_HANDLE) {
        if(app_timer_cancel_event(app, update_timer)) {
            update_timer = APP_TIMER_INVALID_HANDLE;
        }
    }
    // Slow update down to once a second to save power
    update_timer = app_timer_send_event(app, 1000, TIMER_UPDATE);
}

void start_stopwatch() {
    started = true;
    last_pebble_time = 0;
    start_time = 0;
    // Up the resolution to do deciseconds
    if(update_timer != APP_TIMER_INVALID_HANDLE) {
        if(app_timer_cancel_event(app, update_timer)) {
            update_timer = APP_TIMER_INVALID_HANDLE;
        }
    }
    update_timer = app_timer_send_event(app, 100, TIMER_UPDATE);
}

void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window) {
    if(started) {
        stop_stopwatch();
    } else {
        start_stopwatch();
    }
}

void reset_stopwatch_handler(ClickRecognizerRef recognizer, Window *window) {
    if(busy_animating) return;
    bool is_running = started;

    //  if reset what to  reset too
    // moved this as now have watch mode and reset won't do anything in watch mode
    // instead allow inverting is the plan
    int reset_time = config_time;

    switch(appmode)
    {
	case STOPWATCH:
	case YACHTIMER:
	    // if in yachtimer reset to standard times
	    reset_time=start_gun_time;
	case COUNTDOWN:
            countdown_time = reset_time ;
	    stop_stopwatch();
	    buzzatbluepeter=true; 
	    buzzatone=true;
	    elapsed_time = 0;
	    start_time = 0;
	    last_lap_time = 0;
	    last_pebble_time = 0;
	    if(is_running) start_stopwatch();
	    update_stopwatch();

	    // Animate all the laps away.
	    busy_animating = LAP_TIME_SIZE;
	    static PropertyAnimation animations[LAP_TIME_SIZE];
	    static GRect targets[LAP_TIME_SIZE];
	    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
		shift_lap_layer(&animations[i], &lap_layers[i].layer, &targets[i], LAP_TIME_SIZE);
		animation_schedule(&animations[i].animation);
	    }
	    next_lap_layer = 0;
	    break;
	default:
	    // if not in config mode won't do anything which makes this easy
            config_watch(appmode,1);
    }
}

void lap_time_handler(ClickRecognizerRef recognizer, Window *window) {
    if(busy_animating) return;
    if(started)
    {
	    time_t t=0;
	    switch(appmode)
	    {
	    	case STOPWATCH:
			t = elapsed_time - last_lap_time;
			last_lap_time = elapsed_time;
	    		save_lap_time(t);
			break;
		case YACHTIMER:
		case COUNTDOWN:	
			// Save when button was pressed for laptime display
			t = countdown_time - elapsed_time; 	
			last_lap_time = t;

			if(appmode==YACHTIMER)
			{

				// Now target new gun as started if above 2 mins target 4
				// do this even if halted
				if(t >= switch_4or1_time)
				{
					// as manually set don't buzz
					buzzatbluepeter=false;
					elapsed_time = blue_peter_time;
					// had to have been 60 seconds before happened
					start_time = last_pebble_time - (start_gun_time - blue_peter_time);
				}
				else  // otherwise target 1 minute
				{
					// as manually set don't buzz
					buzzatone=false;
					elapsed_time = one_minute_time;

					// had to have been 240 seconds before hadn
					start_time = last_pebble_time - (start_gun_time - one_minute_time) ;
				}
			}
	    		save_lap_time(t);
	    }
    }
    // Update countdown if desired 
    // Note if running adjust elapsed if stopped config
    // does nothing if not in config mode.
    config_watch(appmode,-1);

    // as timer always going will naturally update
}
void set_layer_colours()
{
	if(white_on_black){
		foreground_colour = GColorWhite;
		background_colour = GColorBlack;
		inv_offset = 0;
	}
	else
	{
		foreground_colour = GColorBlack;
		background_colour = GColorWhite;
		// offset pointer used with app mode to work out which bit map to show.
		inv_offset = INVOFFSET;
	}
	
    text_layer_set_background_color(&big_time_layer, background_colour);
    text_layer_set_text_color(&big_time_layer, foreground_colour);
    text_layer_set_background_color(&seconds_time_layer, background_colour);
    text_layer_set_text_color(&seconds_time_layer, foreground_colour);
    text_layer_set_background_color(&watch_layer_date, background_colour);
    text_layer_set_text_color(&watch_layer_date, foreground_colour);
    text_layer_set_background_color(&watch_layer_timebig, background_colour);
    text_layer_set_text_color(&watch_layer_timebig, foreground_colour);
    text_layer_set_background_color(&watch_layer_ampm, background_colour);
    text_layer_set_text_color(&watch_layer_ampm, foreground_colour);
    // Background is clear so don't need to set it.
    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
        text_layer_set_text_color(&lap_layers[i], foreground_colour);
    }
    layer_mark_dirty(&line_layer);
    window_set_background_color(&window, background_colour);
    show_buttons();
}
void config_watch(int appmode,int increment)
{
    int adjustnum = 0;

    // even if running allow minute changes
    switch(appmode)
	{
	case CNTDWNCNFGHRS:
		adjustnum=one_minute_time * 60;
		break;
	// Ok so we want to lower countdown config 
	// Down in increments of 1 minute
	case CNTDWNCNFGMIN:
		adjustnum=one_minute_time;
		break;

	// Seconds
	case CNTDWNCNFGSEC:
		adjustnum=one_second_time;
		break;

	// Allow everything to invert
	case WATCH:
		// Toggle mode
		white_on_black = white_on_black?false:true;
		inv_offset = white_on_black ? 0:INVOFFSET;
		// Do the colour on the layers
		set_layer_colours();
		break;
  	}

	/* for non adjust appmodes does nothing as adjustnum is 0 */
	time_t new_time=0;

	/* if running adjust running time otherwise adjust config time */
	if(started)
	{
		new_time =  elapsed_time + (increment * adjustnum );
		/* if decrementing  stop at lowest value  */
		if(new_time <= 0 ) new_time = elapsed_time;
		elapsed_time = new_time;
	}
	else
	{
		new_time =  config_time + (increment * adjustnum );
		/* if decrementing  stop at lowest value  */
		if(new_time <= 0 ) new_time = config_time;
		config_time = new_time;
		countdown_time = config_time;

	}


}


void update_stopwatch() {
    // max format of %r time format only support %r and %T
    // http://www.cplusplus.com/reference/ctime/strftime/
    // is formats supported
    static char date[] = "Mon Sep 31";
    static char time[] = "00:00";
    static char ampm[] = "  ";
    PblTm timeforformat;
    static char big_time[] = "00:00";
    static char deciseconds_time[] = ".0";
    static char seconds_time[] = ":00";
    time_t display_time = 0;

    //

    switch(appmode)
    {
	// Only do this in watch mode 
	// As timer is fime grained avoiding call
	// That could go to hardware
	case WATCH:
		get_time(&timeforformat);
		string_format_time(date, sizeof(date) , "%a %h %e", &timeforformat);	

		// Don't bother redrawing if correct
		if(strcmp(date,text_layer_get_text(&watch_layer_date)))
		{
    			text_layer_set_text(&watch_layer_date, date);
		}
		
		// formt time
		if ( clock_is_24h_style())
		{
		 	string_format_time(time, sizeof(time) , "%R", &timeforformat);	
		}
		else
		{
			string_format_time(time, sizeof(time) , "%I:%M", &timeforformat);	
			string_format_time(ampm, sizeof(ampm) , "%p", &timeforformat);	
	
			// Don't bother redrawing if correct
			if(strcmp(ampm,text_layer_get_text(&watch_layer_ampm)))
			{
				text_layer_set_text(&watch_layer_ampm, ampm);
			}
		}

		// Don't bother redrawing if correct
		if(strcmp(time,text_layer_get_text(&watch_layer_timebig)))
		{
    			text_layer_set_text(&watch_layer_timebig, time);
		}
		break;
	case STOPWATCH:
		// assume timer does not update elapsed time if not running
		display_time = elapsed_time;
		break;
	// Some form of countdown still do logic 
	default:
		display_time = (elapsed_time > countdown_time) ? 0:(countdown_time - elapsed_time);
		if(appmode==YACHTIMER)
		{
			if(buzzatbluepeter && display_time <= blue_peter_time) 
			{
				buzzatbluepeter=false;
				vibes_double_pulse();
			}
			if(buzzatone && display_time <= one_minute_time) 
			{
				buzzatone=false;
				vibes_double_pulse();
			}
		}
		if(display_time == 0)
		{
			stop_stopwatch();
			vibes_enqueue_custom_pattern(start_pattern);
		}
    } 
    // Now convert to hours/minutes/seconds.
    int tenths = (display_time / 100) % 10;
    int seconds = (display_time / 1000) % 60;
    int minutes = (display_time / 60000) % 60;
    int hours = display_time / 3600000;

    // We can't fit three digit hours, so stop timing here.
    if(hours > 99) {
	stop_stopwatch();
	return;
    }
    if(hours < 1)
    {
        itoa2(minutes, &big_time[0]);
        itoa2(seconds, &big_time[3]);
        itoa1(tenths, &deciseconds_time[1]);
    }
    else
    {
        itoa2(hours, &big_time[0]);
        itoa2(minutes, &big_time[3]);
        itoa2(seconds, &seconds_time[1]);
    }
	
    // Now draw the strings.
    text_layer_set_text(&big_time_layer, big_time);
    text_layer_set_text(&seconds_time_layer, hours < 1 ? deciseconds_time : seconds_time);
}

void animation_stopped(Animation *animation, void *data) {
    --busy_animating;
}

void shift_lap_layer(PropertyAnimation* animation, Layer* layer, GRect* target, int distance_multiplier) {
    GRect origin = layer_get_frame(layer);
    *target = origin;
    target->origin.y += target->size.h * distance_multiplier;
    property_animation_init_layer_frame(animation, layer, NULL, target);
    animation_set_duration(&animation->animation, 250);
    animation_set_curve(&animation->animation, AnimationCurveLinear);
    animation_set_handlers(&animation->animation, (AnimationHandlers){
        .stopped = (AnimationStoppedHandler)animation_stopped
    }, NULL);
}

void save_lap_time(time_t lap_time) {
    if(busy_animating) return;

    static PropertyAnimation animations[LAP_TIME_SIZE];
    static GRect targets[LAP_TIME_SIZE];

    // Shift them down visually (assuming they actually exist)
    busy_animating = LAP_TIME_SIZE;
    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
        if(i == next_lap_layer) continue; // This is handled separately.
        shift_lap_layer(&animations[i], &lap_layers[i].layer, &targets[i], 1);
        animation_schedule(&animations[i].animation);
    }

    // Once those are done we can slide our new lap time in.
    format_lap(lap_time, lap_times[next_lap_layer]);

    // Animate it
    static PropertyAnimation entry_animation;
    //static GRect origin; origin = ;
    //static GRect target; target = ;
    property_animation_init_layer_frame(&entry_animation, &lap_layers[next_lap_layer].layer, &GRect(-139, 52, 139, 26), &GRect(5, 52, 139, 26));
    animation_set_curve(&entry_animation.animation, AnimationCurveEaseOut);
    animation_set_delay(&entry_animation.animation, 50);
    animation_set_handlers(&entry_animation.animation, (AnimationHandlers){
        .stopped = (AnimationStoppedHandler)animation_stopped
    }, NULL);
    animation_schedule(&entry_animation.animation);
    next_lap_layer = (next_lap_layer + 1) % LAP_TIME_SIZE;

    // Get it into the laps window, too.
    store_lap_time(lap_time);
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
    (void)handle;
    if(cookie == TIMER_UPDATE) {
        time_t pebble_time = get_pebble_time();
        if(!last_pebble_time) last_pebble_time = pebble_time;
        if(started) {
            elapsed_time += 100;
            if(pebble_time > last_pebble_time) {
                // If it's the first tick, instead of changing our time we calculate the correct time.
                if(!start_time) {
                    start_time = pebble_time - elapsed_time;
                } else {
                    elapsed_time = pebble_time - start_time;
                }
            }
            update_timer = app_timer_send_event(ctx, elapsed_time <= 3600000 ? 100 : 1000, TIMER_UPDATE);
        }
	else
	{
            update_timer = app_timer_send_event(ctx, 1000, TIMER_UPDATE);
	}
        last_pebble_time = pebble_time;
        update_stopwatch();
    }
}

void handle_display_lap_times(ClickRecognizerRef recognizer, Window *window) {
    show_laps();
}
void show_buttons()
{

  for(int i=0;i<(MAXMODE*2);i++) {
	layer_set_hidden( &button_labels[i].layer.layer, ((appmode+inv_offset)==i?false:true));
  }
}

// Toggle stopwatch timer mode
void toggle_mode(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  Layer *root_layer = window_get_root_layer(window);
  

  // Modes are contigous so each press cycles.
  appmode++;
  if(appmode>=MAXMODE) 
  {
	appmode = 0;
  }
  show_buttons();
  switch(appmode)
  {
	case YACHTIMER:
		countdown_time=start_gun_time;
		break;
	case COUNTDOWN:
		countdown_time=config_time;
		break;
        default:
		;
  }
 
  //get time being shown and not countdown when move to WATCH mode
  if(appmode==WATCH)
  {
	  layer_set_hidden(&watch_layer_date.layer,false);
	  layer_set_hidden(&watch_layer_timebig.layer,false);
	  layer_set_hidden(&watch_layer_ampm.layer,false);
	  layer_set_hidden(&big_time_layer.layer,true);
	  layer_set_hidden(&seconds_time_layer.layer,true);
	  // Background is clear so don't need to set it.
	  for(int i = 0; i < LAP_TIME_SIZE; ++i) {
		layer_set_hidden(&lap_layers[i].layer, true);
	  }
  }
  else
  {
	  layer_set_hidden(&watch_layer_date.layer,true);
	  layer_set_hidden(&watch_layer_timebig.layer,true);
	  layer_set_hidden(&watch_layer_ampm.layer,true);
	  layer_set_hidden(&big_time_layer.layer,false);
	  layer_set_hidden(&seconds_time_layer.layer,false);
	  // Background is clear so don't need to set it.
	  for(int i = 0; i < LAP_TIME_SIZE; ++i) {
		layer_set_hidden(&lap_layers[i].layer, false);
	  }
  }
  update_stopwatch();

}

void config_provider(ClickConfig **config, Window *window) {
    config[BUTTON_RUN]->click.handler = (ClickHandler)toggle_stopwatch_handler;
    config[BUTTON_RUN]->long_click.handler = (ClickHandler) toggle_mode;
    config[BUTTON_RESET]->click.handler = (ClickHandler)reset_stopwatch_handler;
    config[BUTTON_LAP]->click.handler = (ClickHandler)lap_time_handler;
    config[BUTTON_LAP]->long_click.handler = (ClickHandler)handle_display_lap_times;
    config[BUTTON_LAP]->long_click.delay_ms = 700;
    (void)window;
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .timer_handler = &handle_timer
  };
  app_event_loop(params, &handlers);
}
