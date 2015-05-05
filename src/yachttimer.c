 /*
 * Pebble yachtTimer - the big, ugly file.
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

// What does most of the work
#include "yachtimermodel.h"
#include "laps.h"
#include "common.h"


// Removed for SDK 2.0
//
// #define MY_UUID { 0xE7, 0xB1, 0xD0, 0x1B, 0x7F, 0x1C, 0x48, 0x6A, 0x85, 0xB0, 0xDC, 0xA3, 0xD7, 0x4E, 0x7D, 0xCA }
// 
// PBL_APP_INFO(MY_UUID,
//              "YachtTimer", "Mike Moore",
//             5, 4, /* App version */
//             RESOURCE_ID_IMAGE_MENU_ICON,
//             APP_INFO_STANDARD_APP);
//

uint32_t storageKey = DEFAULTYMSTORAGEKEY;

#define LAP_TIME_SIZE 5
#define LAP_TIME_LEN 11
/*  happens to be in the model
#define	YACHTIMER	0
#define	STOPWATCH	1
#define COUNTDOWN	2
*/
#define CNTDWNCNFGHRS	3
#define CNTDWNCNFGMIN	4
#define CNTDWNCNFGSEC	5
#define WATCH		6



// Lets start with one timer
// as oo can now add as many as you want.
YachtTimer *myYachtTimer;



// Ok so view and controller from now on
#define MAXMODE		7
#define MAX_TIME	((ASECOND * 59)+(ASECOND * 59 * 60)+(ASECOND * 24 * 6 * 60 * 60)) // 6 days 24 hours 59 minutes and 59 seconds over a 144 Hours

// View and controller
static Window *window;
// static AppTimer *app;

// Main display
static TextLayer *big_time_layer;
static TextLayer *watch_layer_date;
static TextLayer *watch_layer_timebig;
static TextLayer *watch_layer_ampm;
static TextLayer *seconds_time_layer;
static Layer *line_layer;


// while many clocks display only 
static 	char lap_times[LAP_TIME_SIZE][LAP_TIME_LEN] = {"00:00:00.0", "00:01:00.0", "00:02:00.0", "00:03:00.0", "00:04:00.0"};

// Lap time display
static TextLayer *lap_layers[LAP_TIME_SIZE]; // an extra temporary layer
static int next_lap_layer = 0;

// Now we do the model and cosntants for the model.
// This will get segregated out.
// Aim of segregating model is to allow unlimited types of stopwatches, timers etc.
// as no malloc will limit memory used using constants

// The documentation claims this is defined, but it is not.
// Define it here for now.
#ifndef APP_TIMER_INVALID_HANDLE
    #define APP_TIMER_INVALID_HANDLE 0xDEADBEEF
#endif

// Actually keeping track of time
static AppTimer *update_timer = NULL;

// Define starting colour mode.
#define WHITE_ON_BLACK
#define INVOFFSET	MAXMODE

#ifdef WHITE_ON_BLACK
static bool white_on_black=true;
static GColor foreground_colour;  // = GColorWhite;
static GColor background_colour; //  = GColorBlack;
// offset pointer used with app mode to work out which bit map to show.
static int inv_offset = 0;
#else
static bool white_on_black=false;
static GColor foreground_colour; // = GColorBlack;
static GColor background_colour; // = GColorWhite;
// offset pointer used with app mode to work out which bit map to show.
static int inv_offset = INVOFFSET;
#endif

// Have inverted as second set of bitmaps
// so use max multiplier
static int buttonmodeimages[MAXMODE * 2];
static BitmapLayer *button_labels[MAXMODE * 2];
static GBitmap  *button_images[MAXMODE * 2];


// What we start in 
static int startappmode = YACHTIMER;
static int watchappmode  = YACHTIMER ;
static GRect savelayerpos1, savelayerpos2;

// Custom vibration pattern
const VibePattern start_pattern = {
  .durations = (uint32_t []) {100, 300, 300, 300, 100, 300},
  .num_segments = 6
};

// Global animation lock. As long as we only try doing things while
// this is zero, we shouldn't crash the watch.
static int busy_animating = 0;
static int ticklen=0;
static GFont big_font, seconds_font, laps_font;

#define TIMER_UPDATE 1
#define FONT_BIG_TIME RESOURCE_ID_FONT_DEJAVU_SANS_BOLD_SUBSET_30
#define FONT_SECONDS RESOURCE_ID_FONT_DEJAVU_SANS_SUBSET_18
#define FONT_LAPS RESOURCE_ID_FONT_DEJAVU_SANS_SUBSET_22

#define BUTTON_LAP BUTTON_ID_DOWN
#define BUTTON_RUN BUTTON_ID_SELECT
#define BUTTON_RESET BUTTON_ID_UP

// View an dcontroler method
// void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void countdown_config_handler(ClickRecognizerRef recognizer, void *data);
void config_provider(void *context);
void handle_init();
time_t time_seconds();
void show_buttons();
void set_layer_colours();
void stop_stopwatch();
void start_stopwatch();
void toggle_stopwatch_handler(ClickRecognizerRef recognizer, void *data);
void toggle_mode(ClickRecognizerRef recognizer, void *data);
void reset_stopwatch_handler(ClickRecognizerRef recognizer, void *data);
// main view method
void update_stopwatch();

// Hook to ticks
void handle_timer(void *data);
// void pbl_main(void *params);
void draw_line(Layer *me, GContext* ctx);
void save_lap_time(time_t seconds);
void lap_time_handler(ClickRecognizerRef recognizer, void *data);
void shift_lap_layer(Animation* animation, Layer* layer, GRect* target, int distance_multiplier);
void config_watch(int appmode,int increment);

// Animation handlers
void animation_stopped(Animation *animation, void *data);

#ifdef PBL_SDK_3
static StatusBarLayer *s_status_bar;
static struct Layer *s_battery_layer;
#endif
#ifdef PBL_SDK_3
static void battery_proc(Layer *layer, GContext *ctx) {
  // Emulator battery meter on Aplite
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(126, 4, 14, 8));
  graphics_draw_line(ctx, GPoint(140, 6), GPoint(140, 9));

  BatteryChargeState state = battery_state_service_peek();
  int width = (int)(float)(((float)state.charge_percent / 100.0F) * 10.0F);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(128, 6, width, 4), 0, GCornerNone);
}
#endif
static void main_window_load(Window *window) {

  /* other UI code */

#ifdef PBL_SDK_3

  // Set up the status bar last to ensure it is on top of other Layers
  struct Layer *window_layer = window_get_root_layer(window);
  s_status_bar = status_bar_layer_create();
  layer_add_child(window_layer , status_bar_layer_get_layer(s_status_bar));

  // Show legacy battery meter
  GRect bounds = layer_get_bounds(status_bar_layer_get_layer(s_status_bar));
  s_battery_layer = layer_create(GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, STATUS_BAR_LAYER_HEIGHT));
  layer_set_update_proc(s_battery_layer, battery_proc);
  layer_add_child(window_layer, s_battery_layer);	

#endif
}
void handle_init() {

#ifdef WHITE_ON_BLACK
foreground_colour = GColorWhite;
background_colour = GColorBlack;
// offset pointer used with app mode to work out which bit map to show.
#else
foreground_colour = GColorBlack;
background_colour = GColorWhite;
// offset pointer used with app mode to work out which bit map to show.
#endif


    window = window_create();
    // window_init(&window, "Stopwatch");
    window_stack_push(window, true /* Animated */);
    window_set_background_color(window, background_colour);
#ifdef PBL_SDK_3
    window_set_window_handlers(window, (WindowHandlers){
        .load = (WindowHandler)main_window_load
    });
#else
    window_set_fullscreen(window, false);
#endif

    // resource_init_current_app(&APP_RESOURCES);

    // Arrange for user input.
    window_set_click_config_provider(window,  config_provider);

    // Get our fonts
    big_font = fonts_load_custom_font(resource_get_handle(FONT_BIG_TIME));
    seconds_font = fonts_load_custom_font(resource_get_handle(FONT_SECONDS));
    laps_font = fonts_load_custom_font(resource_get_handle(FONT_LAPS));

    // Root layer
    Layer *root_layer = window_get_root_layer(window);

    // Set up the big timer.
    big_time_layer = text_layer_create(GRect(0, 5, 96, 35));
    // text_layer_init(&big_time_layer, GRect(0, 5, 96, 35));
    text_layer_set_background_color(big_time_layer, background_colour);
    text_layer_set_font(big_time_layer, big_font);
    text_layer_set_text_color(big_time_layer, foreground_colour);

    
    // in init only have either stopwatch as count up  or other modes count downs
    // if countdown then default to 5 mins same as yacht timer anyhow
    myYachtTimer = yachtimer_create(startappmode);
    startappmode = yachtimer_getMode(myYachtTimer);;

    if(startappmode == STOPWATCH)
    {
    	text_layer_set_text(big_time_layer, "00:00");
    }
    else
    {
    	text_layer_set_text(big_time_layer, "05:00");
    }
    text_layer_set_text_alignment(big_time_layer, GTextAlignmentRight);
    layer_add_child(root_layer, (Layer *)big_time_layer);

    seconds_time_layer = text_layer_create(GRect(96, 17, 49, 35));

    // text_layer_init(&seconds_time_layer, GRect(96, 17, 49, 35));
    text_layer_set_background_color(seconds_time_layer, background_colour);
    text_layer_set_font(seconds_time_layer, seconds_font);
    text_layer_set_text_color(seconds_time_layer, foreground_colour);
    text_layer_set_text(seconds_time_layer, ".0");
    layer_add_child(root_layer, (Layer *)seconds_time_layer);

    // Set up the watch layer but hide it.
    watch_layer_date = text_layer_create(GRect(0, 12, 96+49, 35));

    // text_layer_init(&watch_layer_date, GRect(0, 12, 96+49, 35));
    text_layer_set_background_color(watch_layer_date, background_colour);
    text_layer_set_font(watch_layer_date, laps_font);
    text_layer_set_text_color(watch_layer_date, foreground_colour);

    // ensure full size layer with 12 hour format.
    text_layer_set_text(watch_layer_date, "September 31");
    text_layer_set_text_alignment(watch_layer_date, GTextAlignmentLeft);

    // by default hidden switch with big and little in watch mode
    layer_set_hidden((Layer *)watch_layer_date,true);
    layer_add_child(root_layer, (Layer *)watch_layer_date);

    // Set up the watch layer but hide it.
    watch_layer_timebig = text_layer_create(GRect(0, 52, 96, 35));

    // text_layer_init(&watch_layer_timebig, GRect(0, 52, 96, 35));
    text_layer_set_background_color(watch_layer_timebig, background_colour);
    text_layer_set_font(watch_layer_timebig, big_font);
    text_layer_set_text_color(watch_layer_timebig, foreground_colour);

    // ensure full size layer with 12 hour format.
    text_layer_set_text(watch_layer_timebig, "00:00");
    text_layer_set_text_alignment(watch_layer_timebig, GTextAlignmentLeft);

    // by default hidden switch with big and little in watch mode
    layer_set_hidden((Layer *)watch_layer_timebig,true);
    layer_add_child(root_layer, (Layer *)watch_layer_timebig);

    // Set up the watch layer but hide it. 
    watch_layer_ampm = text_layer_create(GRect(96, 60, 49, 35));
    // text_layer_init(&watch_layer_ampm, GRect(96, 60, 49, 35));
    text_layer_set_background_color(watch_layer_ampm, background_colour);
    text_layer_set_font(watch_layer_ampm, laps_font);
    text_layer_set_text_color(watch_layer_ampm, foreground_colour);

    // ensure full size layer with  am/pm
    text_layer_set_text(watch_layer_ampm, "AM");
    text_layer_set_text_alignment(watch_layer_ampm, GTextAlignmentLeft);

    // by default hidden switch with big and little in watch mode
    layer_set_hidden((Layer *)watch_layer_ampm,true);
    layer_add_child(root_layer, (Layer *)watch_layer_ampm);

    // Draw our nice line.
    line_layer = layer_create(GRect(0, 45, 144, 2));
    // layer_init(&line_layer, GRect(0, 45, 144, 2));
    layer_set_update_proc(line_layer, draw_line);
    // line_layer.update_proc = &draw_line;
    layer_add_child(root_layer, line_layer);

    // Set up the lap time layers. These will be made visible later.
    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
	lap_layers[i] = text_layer_create(GRect(-139, 52, 139, 30));
        // text_layer_init(&lap_layers[i], GRect(-139, 52, 139, 30));
        text_layer_set_background_color(lap_layers[i], GColorClear);
        text_layer_set_font(lap_layers[i], laps_font);
        text_layer_set_text_color(lap_layers[i], foreground_colour);
        text_layer_set_text(lap_layers[i], lap_times[i]);
        layer_add_child(root_layer, (Layer *)lap_layers[i]);
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
    for(int i=0;i<(MAXMODE * 2) ;i++) 
    {
	button_images[i] = gbitmap_create_with_resource	(buttonmodeimages[i]); 	
        button_labels[i] = bitmap_layer_create(GRect(130, 10, 14, 136));
	bitmap_layer_set_bitmap(button_labels[i],button_images[i]);
    	// bmp_init_container(buttonmodeimages[i], &button_labels[i]);
    	// layer_set_frame(&button_labels[i].layer.layer, GRect(130, 10, 14, 136));

	// Make sure active mode button only visible
	layer_set_hidden( (Layer *) button_labels[i], ((startappmode+inv_offset)==i?false:true));

	// add child layers to root_layer
    	layer_add_child(root_layer, (Layer *) button_labels[i]);
    }
    // save position of stop watch / timer
    savelayerpos1 = layer_get_frame((Layer *)big_time_layer);
    savelayerpos2 = layer_get_frame((Layer *)seconds_time_layer);
    

    // Set up lap time stuff, too.
    init_lap_window();

    // Get timer going 
    ticklen = yachtimer_getTick(myYachtTimer);
    static uint32_t cookie = TIMER_UPDATE;
    update_timer = app_timer_register( ticklen, &handle_timer, &cookie );
}


void handle_deinit() {
    for(int i=0;i<(MAXMODE*2);i++)
    {
	bitmap_layer_destroy(button_labels[i]);
	gbitmap_destroy(button_images[i]);
    }

    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
	text_layer_destroy(lap_layers[i]);
    }
    text_layer_destroy(big_time_layer);
    text_layer_destroy(seconds_time_layer);
    text_layer_destroy(watch_layer_date);
    text_layer_destroy(watch_layer_timebig);
    text_layer_destroy(watch_layer_ampm);
    layer_destroy(line_layer);
    fonts_unload_custom_font(big_font);
    fonts_unload_custom_font(seconds_font);
    fonts_unload_custom_font(laps_font);
    yachtimer_destroy(myYachtTimer);
    window_destroy(window);
}

void draw_line(Layer *me, GContext* ctx) {
    graphics_context_set_stroke_color(ctx, foreground_colour);
    graphics_draw_line(ctx, GPoint(0, 0), GPoint(140, 0));
    graphics_draw_line(ctx, GPoint(0, 1), GPoint(140, 1));
}

void stop_stopwatch() {
   
    yachtimer_stop(myYachtTimer); 
    if(update_timer != NULL) {
        app_timer_cancel( update_timer);
        update_timer = NULL;
    }
    // Slow update down to once a second to save power
    ticklen = yachtimer_getTick(myYachtTimer);
    static uint32_t cookie = TIMER_UPDATE;
    update_timer = app_timer_register( ticklen, &handle_timer, &cookie );
}

void start_stopwatch() {
    yachtimer_start(myYachtTimer);

    // default start mode
    startappmode = yachtimer_getMode(myYachtTimer);;

    // Up the resolution to do deciseconds
    if(update_timer != NULL) {
        app_timer_cancel(update_timer);
        update_timer = NULL;
    }
    static uint32_t cookie = TIMER_UPDATE;
    update_timer = app_timer_register( yachtimer_getTick(myYachtTimer), &handle_timer, &cookie);
}

void toggle_stopwatch_handler(ClickRecognizerRef recognizer, void *data) {
    if(yachtimer_isRunning(myYachtTimer)) {
        stop_stopwatch();
    } else {
        start_stopwatch();
    }
}

void reset_stopwatch_handler(ClickRecognizerRef recognizer, void *data) {
    if(busy_animating) return;

    yachtimer_reset(myYachtTimer);

    switch(watchappmode)
    {
	case STOPWATCH:
	case YACHTIMER:
	case COUNTDOWN:
	    if(yachtimer_isRunning(myYachtTimer))
	    {
		 stop_stopwatch();
		 start_stopwatch();
	    }
	    else
	    {
	    	stop_stopwatch();
	    }
	    update_stopwatch();

	    // Animate all the laps away.
	    busy_animating = LAP_TIME_SIZE;
	    static PropertyAnimation *animations[LAP_TIME_SIZE];
	    static GRect targets[LAP_TIME_SIZE];
            static bool first=true;
	    GRect origin;
	    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
		origin = layer_get_frame((Layer *)lap_layers[i]);
		targets[i] = origin;
		targets[i].origin.y += targets[i].size.h * LAP_TIME_SIZE;
                if(!first && animations[i] != NULL)
                {
			property_animation_destroy(animations[i]);
                }
		animations[i] = property_animation_create_layer_frame((Layer *)lap_layers[i],NULL,&targets[i]);
		shift_lap_layer((Animation *)animations[i], (Layer *)lap_layers[i], &targets[i], LAP_TIME_SIZE);
		animation_schedule((Animation *)animations[i]);
	    }
            first = false;
	    next_lap_layer = 0;
	    break;
	default:
	    // if not in config mode won't do anything which makes this easy
            config_watch(watchappmode,1);
    }
}

void lap_time_handler(ClickRecognizerRef recognizer, void *data) {
    time_t t=0;
    if(busy_animating) return;

    // if not running will retunr 0 which is useless
    // so check timer is running before diaplaying stuff
    if(yachtimer_isRunning(myYachtTimer))
    {
	    // returns laptime of current mode
	    // if overrun timer willbe time since overrun started
	    t=labs(yachtimer_getLap(myYachtTimer));
	    switch(watchappmode)
	    {
	    	case STOPWATCH:
		case YACHTIMER:
		case COUNTDOWN:	
	    		save_lap_time(t);
	    }
    }
    // Update countdown if desired 
    // Note if running adjust elapsed if stopped config
    // does nothing if not in config mode.
    config_watch(watchappmode,-1);

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
	
    text_layer_set_background_color(big_time_layer, background_colour);
    text_layer_set_text_color(big_time_layer, foreground_colour);
    text_layer_set_background_color(seconds_time_layer, background_colour);
    text_layer_set_text_color(seconds_time_layer, foreground_colour);
    text_layer_set_background_color(watch_layer_date, background_colour);
    text_layer_set_text_color(watch_layer_date, foreground_colour);
    text_layer_set_background_color(watch_layer_timebig, background_colour);
    text_layer_set_text_color(watch_layer_timebig, foreground_colour);
    text_layer_set_background_color(watch_layer_ampm, background_colour);
    text_layer_set_text_color(watch_layer_ampm, foreground_colour);
    // Background is clear so don't need to set it.
    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
        text_layer_set_text_color(lap_layers[i], foreground_colour);
    }
    layer_mark_dirty(line_layer);
    window_set_background_color(window, background_colour);
    show_buttons();
}
void config_watch(int appmode,int increment)
{
    int adjustnum = 0;

    // even if running allow minute changes
    switch(appmode)
	{
	case CNTDWNCNFGHRS:
		adjustnum=ASECOND * 60 * 60;
		break;
	// Ok so we want to lower countdown config 
	// Down in increments of 1 minute
	case CNTDWNCNFGMIN:
		adjustnum=ASECOND * 60;
		break;

	// Seconds
	case CNTDWNCNFGSEC:
		adjustnum=ASECOND;
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


void update_stopwatch() {
    // max format of %r time format only support %r and %T
    // http://www.cplusplus.com/reference/ctime/strftime/
    // is formats supported
    static char date[] = "Mon Sep 31";
    static char time[] = "00:00";
    static char ampm[] = "  ";

    // default display using appmode but if in watch mode display in started mode.
    int stopwatchappmode = watchappmode;
    struct tm  *timeforformat;
    struct tm  *timerforformat;
    static char big_time[] = "00:00";
    static char deciseconds_time[] = ".0";
    static char seconds_time[] = ":00";
    time_t display_time = 0;
    


    switch(stopwatchappmode)
    {
	// Only do this in watch mode 
	// As timer is fime grained avoiding call
	// That could go to hardware
	case WATCH:
		timeforformat = yachtimer_getPblLastTime(myYachtTimer);
		strftime(date, sizeof(date) , "%a %h %e", timeforformat);	

		// Don't bother redrawing if correct
		if(strcmp(date,text_layer_get_text(watch_layer_date)))
		{
    			text_layer_set_text(watch_layer_date, date);
		}
		
		// formt time
		if ( clock_is_24h_style())
		{
		 	strftime(time, sizeof(time) , "%R", timeforformat);	
		}
		else
		{
			strftime(time, sizeof(time) , "%I:%M", timeforformat);	
			strftime(ampm, sizeof(ampm) , "%p", timeforformat);	
	
		}
		// Don't bother redrawing if correct
		if(strcmp(ampm,text_layer_get_text(watch_layer_ampm)))
		{
			text_layer_set_text(watch_layer_ampm, ampm);
		}

		// Don't bother redrawing if correct
		if(strcmp(time,text_layer_get_text(watch_layer_timebig)))
		{
    			text_layer_set_text(watch_layer_timebig, time);
		}
		// shows the mode that a counter was started in when in watch mode.
		// otherwise always shows last which is countdown.
		// 
		yachtimer_setMode(myYachtTimer,startappmode);
	//  Some interesting implied logic as default fall through is some form of countdown in config mode
        // as in config stopwatch appmode is left as is
    }

    // abs flips negative times possible in countdown starts to show time since
    // countdown completed
    display_time = labs(yachtimer_getDisplayTime(myYachtTimer));

    // Get a time we can format
    timerforformat = yachtimer_getPblDisplayTime(myYachtTimer);

    theTimeEventType event = yachtimer_triggerEvent(myYachtTimer);

    if(event == MinorTime) vibes_double_pulse();
    if(event == MajorTime) vibes_enqueue_custom_pattern(start_pattern);

    // Now convert to hours/minutes/seconds.
    time_t tenths = (display_time / DECISECOND) % 10;
    time_t hours = display_time / (60L * 60L * ASECOND);

    // Cannot do more than 7 days as loops back to 0.
    if(display_time > MAX_TIME) {
	stop_stopwatch();
	return;
    }
    if(hours < 1)
    {
	// show minutes:seconds.tenths
	strftime(big_time, sizeof(big_time) , "%M:%S", timerforformat);	

	// as tenths no weird roll over	
        itoa1(tenths, &deciseconds_time[1]);
    }
    else
    {
	// While less than 24 hours
	if(hours < 24)
	{
		// show hours:minute:seconds
		strftime(big_time, sizeof(big_time) , "%R", timerforformat);	
		strftime(seconds_time, sizeof(seconds_time) , ":%S", timerforformat);	
	}
	// Now show days, hours, minutes
	else
	{
		// Show day of month:hour:minute
		strftime(big_time, sizeof(big_time) , "%w.%H", timerforformat);	
		strftime(seconds_time, sizeof(seconds_time) , ":%M", timerforformat);	
	}
    }
	
    // Now draw the strings.
    text_layer_set_text(big_time_layer, big_time);
    text_layer_set_text(seconds_time_layer, hours < 1 ? deciseconds_time : seconds_time);
}

void animation_stopped(Animation *animation, void *data) {
    --busy_animating;
}
void shift_lap_layer(Animation* animation, Layer* layer, GRect* target, int distance_multiplier) {
    animation_set_duration(animation, 250);
    animation_set_curve(animation, AnimationCurveLinear);
    animation_set_handlers(animation, (AnimationHandlers){
        .stopped = (AnimationStoppedHandler)animation_stopped
    }, NULL);
}

void save_lap_time(time_t lap_time) {
    if(busy_animating) return;

    static PropertyAnimation *animations[LAP_TIME_SIZE];
    static GRect targets[LAP_TIME_SIZE];
    static bool first = true;
    GRect origin;

    // Shift them down visually (assuming they actually exist)
    busy_animating = LAP_TIME_SIZE;
    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
        if(i == next_lap_layer) continue; // This is handled separately.
	origin = layer_get_frame((Layer *)lap_layers[i]);
	targets[i] = origin;
    	targets[i].origin.y += targets[i].size.h * 1;
        if(!first && animations[i] != NULL)
	{
		property_animation_destroy(animations[i] );
	}
	animations[i] = property_animation_create_layer_frame((Layer *)lap_layers[i],NULL,&targets[i]);
        shift_lap_layer((Animation *)animations[i], (Layer *)lap_layers[i], &targets[i], 1);
        animation_schedule((Animation *)animations[i]);
    }
    first = false;

    // Once those are done we can slide our new lap time in.
    format_lap(lap_time, lap_times[next_lap_layer],LAP_TIME_LEN);

    // Animate it
    static PropertyAnimation *entry_animation = NULL;
    //static GRect origin; origin = ;
    //static GRect target; target = ;
    if(entry_animation)
    {
	property_animation_destroy(entry_animation);
    }
    entry_animation = property_animation_create_layer_frame((Layer *)lap_layers[next_lap_layer], &GRect(-139, 52, 139, 26), &GRect(5, 52, 139, 26));
    // property_animation_init_layer_frame(&entry_animation, (Layer *)lap_layers[next_lap_layer], &GRect(-139, 52, 139, 26), &GRect(5, 52, 139, 26));
    animation_set_curve((Animation *)entry_animation, AnimationCurveEaseOut);
    animation_set_delay((Animation *)entry_animation, 50);
    animation_set_handlers((Animation *)entry_animation, (AnimationHandlers){
        .stopped = (AnimationStoppedHandler)animation_stopped
    }, NULL);
    animation_schedule((Animation *)entry_animation);
    next_lap_layer = (next_lap_layer + 1) % LAP_TIME_SIZE;

    // Get it into the laps window, too.
    store_lap_time(lap_time);
}

void handle_timer(void *data) {
    uint32_t cookie = *(uint32_t *)data;
    if(cookie == TIMER_UPDATE) {
	yachtimer_tick(myYachtTimer,ticklen);
        ticklen = yachtimer_getTick(myYachtTimer);
    	update_timer = app_timer_register( ticklen, &handle_timer, data);
        update_stopwatch();
    }
}

void handle_display_lap_times(ClickRecognizerRef recognizer, void *data) {
    show_laps();
}
void show_buttons()
{
  int appmode = watchappmode;

  for(int i=0;i<(MAXMODE*2);i++) {
	layer_set_hidden( (Layer *) button_labels[i], ((appmode+inv_offset)==i?false:true));
  }
}

// Toggle stopwatch timer mode
void toggle_mode(ClickRecognizerRef recognizer, void *data) {
	  
	  watchappmode++;
	  // Modes are contigous so each press cycles.
	  if(watchappmode>=MAXMODE) 
	  {
		watchappmode = 0;
	  }
	  // Can only set to first three but watch appmode can have 6 modes
	  yachtimer_setMode(myYachtTimer,watchappmode);
	 
	  show_buttons();
	 
	  //get time being shown and not countdown when move to WATCH mode
	  if(watchappmode==WATCH)
	  {
		  layer_set_hidden((Layer *)watch_layer_date,false);
		  layer_set_hidden((Layer *)watch_layer_timebig,false);
		  layer_set_hidden((Layer *)watch_layer_ampm,false);

		  // Hide stopwatch/countdown if not started
		  if(!yachtimer_isRunning(myYachtTimer))		
		  {
		  	layer_set_hidden((Layer *)big_time_layer,true);
		  	layer_set_hidden((Layer *)seconds_time_layer,true);
		  }
		  // Background is clear so don't need to set it.
		  for(int i = 0; i < LAP_TIME_SIZE; ++i) {
			layer_set_hidden((Layer *)lap_layers[i], true);
		  }
		  // save position of stop watch / timer
		  savelayerpos1 = layer_get_frame((Layer *)big_time_layer);
		  savelayerpos2 = layer_get_frame((Layer *)seconds_time_layer);

		  // Now move stopwatch/timer mode started will be used as default
		  layer_set_frame((Layer *)big_time_layer,GRect(0,98,96,35));
		  layer_set_frame((Layer *)seconds_time_layer,GRect(96,110,49,35));
	  }
	  else
	  {
		  layer_set_hidden((Layer *)watch_layer_date,true);
		  layer_set_hidden((Layer *)watch_layer_timebig,true);
		  layer_set_hidden((Layer *)watch_layer_ampm,true);
		  layer_set_hidden((Layer *)big_time_layer,false);
		  layer_set_hidden((Layer *)seconds_time_layer,false);
		  // Background is clear so don't need to set it.
		  for(int i = 0; i < LAP_TIME_SIZE; ++i) {
			layer_set_hidden((Layer *)lap_layers[i], false);
		  }
		  // save position of stop watch / timer
		  layer_set_frame((Layer *)big_time_layer,savelayerpos1);
		  layer_set_frame((Layer *)seconds_time_layer,savelayerpos2);
	  }
	  update_stopwatch();

}

void config_provider(void *context) {
    window_single_click_subscribe(BUTTON_RUN, toggle_stopwatch_handler);
    window_long_click_subscribe(BUTTON_RUN, 1000,  toggle_mode, NULL);
    window_single_click_subscribe(BUTTON_RESET, reset_stopwatch_handler);
    window_single_click_subscribe(BUTTON_LAP, lap_time_handler);
    window_long_click_subscribe(BUTTON_LAP, 700,  handle_display_lap_times, NULL);
/*
    config[BUTTON_RUN]->click.handler = (ClickHandler)toggle_stopwatch_handler;
    config[BUTTON_RUN]->long_click.handler = (ClickHandler) toggle_mode;
    config[BUTTON_RESET]->click.handler = (ClickHandler)reset_stopwatch_handler;
    config[BUTTON_LAP]->click.handler = (ClickHandler)lap_time_handler;
    config[BUTTON_LAP]->long_click.handler = (ClickHandler)handle_display_lap_times;
    config[BUTTON_LAP]->long_click.delay_ms = 700;
    (void)window;*/
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}

/*
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .timer_handler = &handle_timer
  };
  app_event_loop(params, &handlers);
}
*/
