#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

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
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
AppContextRef app;

// Main display
TextLayer big_time_layer;
TextLayer seconds_time_layer;
Layer line_layer;

// Custom vibration pattern
const VibePattern start_pattern = {
  .durations = (uint32_t []) {100, 300, 300, 300, 100, 300},
  .num_segments = 6
};


// Lap time display
#define LAP_TIME_SIZE 5
char lap_times[LAP_TIME_SIZE][11] = {"00:00:00.0", "00:01:00.0", "00:02:00.0", "00:03:00.0", "00:04:00.0"};
TextLayer lap_layers[LAP_TIME_SIZE]; // an extra temporary layer
int next_lap_layer = 0;
time_t last_lap_time = 0;

// The documentation claims this is defined, but it is not.
// Define it here for now.
#ifndef APP_TIMER_INVALID_HANDLE
    #define APP_TIMER_INVALID_HANDLE 0xDEADBEEF
#endif

// Actually keeping track of time
time_t elapsed_time = 0;

// We want hundredths of a second, but Pebble won't give us that.
// Pebble's timers are also too inaccurate (we run fast for some reason)
// Instead, we count our own time but also adjust ourselves every pebble
// clock tick. We maintain our original offset in hundredths of a second
// from the first tick. This should ensure that we always have accurate times.
time_t start_time = 0;
time_t last_pebble_time = 0;


// Hard coded 5 minutes 
time_t start_gun_time = 300000;
// 4 minutes
time_t blue_peter_time = 240000;
// 2 minutes focus constant if above go to 4 minutes if below go to 1 minute
time_t switch_4or1_time = 120000;
// 1 minute
time_t one_minute_time = 60000;

// false = countdown, true = stopwatch
// Keep Katherine stopwatch mode
bool isstopwatch = false;

bool started = false;
AppTimerHandle update_timer = APP_TIMER_INVALID_HANDLE;

// Global animation lock. As long as we only try doing things while
// this is zero, we shouldn't crash the watch.
int busy_animating = 0;

#define TIMER_UPDATE 1
#define FONT_BIG_TIME RESOURCE_ID_FONT_DEJAVU_SANS_BOLD_SUBSET_30
#define FONT_SECONDS RESOURCE_ID_FONT_DEJAVU_SANS_SUBSET_18
#define FONT_LAPS RESOURCE_ID_FONT_DEJAVU_SANS_SUBSET_22

void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void config_provider(ClickConfig **config, Window *window);
void handle_init(AppContextRef ctx);
time_t time_seconds();
void stop_stopwatch();
void start_stopwatch();
void toggle_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void toggle_mode(ClickRecognizerRef recognizer, Window *window);
void reset_stopwatch_handler(ClickRecognizerRef recognizer, Window *window);
void itoa2(int num, char* buffer);
void update_stopwatch();
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie);
void pbl_main(void *params);
void draw_line(Layer *me, GContext* ctx);
void save_lap_time(time_t seconds);
void lap_time_handler(ClickRecognizerRef recognizer, Window *window);
void shift_lap_layer(PropertyAnimation* animation, Layer* layer, GRect* target, int distance_multiplier);
time_t get_pebble_time();

void handle_init(AppContextRef ctx) {
    app = ctx;

    window_init(&window, "Stopwatch");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);
    window_set_fullscreen(&window, false);

    resource_init_current_app(&APP_RESOURCES);

    // Arrange for user input.
    window_set_click_config_provider(&window, (ClickConfigProvider) config_provider);

    // Get our fonts
    GFont big_font = fonts_load_custom_font(resource_get_handle(FONT_BIG_TIME));
    GFont seconds_font = fonts_load_custom_font(resource_get_handle(FONT_SECONDS));
    GFont laps_font = fonts_load_custom_font(resource_get_handle(FONT_LAPS));

    // Root layer
    Layer *root_layer = window_get_root_layer(&window);

    // Set up the big timer.
    text_layer_init(&big_time_layer, GRect(0, 5, 96, 35));
    text_layer_set_background_color(&big_time_layer, GColorBlack);
    text_layer_set_font(&big_time_layer, big_font);
    text_layer_set_text_color(&big_time_layer, GColorWhite);
    text_layer_set_text(&big_time_layer, "00:00");
    text_layer_set_text_alignment(&big_time_layer, GTextAlignmentRight);
    layer_add_child(root_layer, &big_time_layer.layer);

    text_layer_init(&seconds_time_layer, GRect(96, 17, 49, 35));
    text_layer_set_background_color(&seconds_time_layer, GColorBlack);
    text_layer_set_font(&seconds_time_layer, seconds_font);
    text_layer_set_text_color(&seconds_time_layer, GColorWhite);
    text_layer_set_text(&seconds_time_layer, ":00.0");
    layer_add_child(root_layer, &seconds_time_layer.layer);

    // Draw our nice line.
    layer_init(&line_layer, GRect(0, 45, 144, 2));
    line_layer.update_proc = &draw_line;
    layer_add_child(root_layer, &line_layer);

    // Set up the lap time layers. These will be made visible later.
    for(int i = 0; i < LAP_TIME_SIZE; ++i) {
        text_layer_init(&lap_layers[i], GRect(-139, 52, 139, 30));
        text_layer_set_background_color(&lap_layers[i], GColorClear);
        text_layer_set_font(&lap_layers[i], laps_font);
        text_layer_set_text_color(&lap_layers[i], GColorWhite);
        text_layer_set_text(&lap_layers[i], lap_times[i]);
        layer_add_child(root_layer, &lap_layers[i].layer);
    }
}


void draw_line(Layer *me, GContext* ctx) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
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
}

// Milliseconds since January 1st 2012 in some timezone, discounting leap years.
// There must be a better way to do this...
time_t get_pebble_time() {
    PblTm t;
    get_time(&t);
    time_t seconds = t.tm_sec;
    seconds += t.tm_min * 60;
    seconds += t.tm_hour * 3600;
    seconds += t.tm_yday * 86400;
    seconds += (t.tm_year - 2012) * 31536000;
    return seconds * 1000;
}

void start_stopwatch() {
    started = true;

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
    elapsed_time = 0;
    start_time=0;
    last_lap_time = 0; 
    last_lap_time = 0;
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
}

void lap_time_handler(ClickRecognizerRef recognizer, Window *window) {
    if(busy_animating) return;
    if(isstopwatch)
    {
     	time_t t = elapsed_time - last_lap_time;
    	last_lap_time = elapsed_time;
    	save_lap_time(t);
    }
    else
    {
	if(started)
	{
		// Save when button was pressed for aptime display
		time_t t = start_gun_time - elapsed_time; 	
		last_lap_time = t;
		save_lap_time(t);

		// Now target new gun as started if above 2 mins target 4
		if(t >= switch_4or1_time)
		{
			elapsed_time = blue_peter_time;
			// had to have been 60 seconds before hadn
			start_time = last_pebble_time - (start_gun_time - blue_peter_time);
		}
		else  // otherwise target 1 minute
		{
			elapsed_time = one_minute_time;

			// had to have been 240 seconds before hadn
			start_time = last_pebble_time - (start_gun_time - one_minute_time) ;
		}
	}
    }
}

// There must be some way of doing this besides writing our own...
void itoa1(int num, char* buffer) {
    const char digits[10] = "0123456789";
    buffer[0] = digits[num % 10];
}
void itoa2(int num, char* buffer) {
    const char digits[10] = "0123456789";
    if(num > 99) {
        buffer[0] = '9';
        buffer[1] = '9';
        return;
    } else if(num > 9) {
        buffer[0] = digits[num / 10];
    } else {
        buffer[0] = '0';
    }
    buffer[1] = digits[num % 10];
}

void update_stopwatch() {
    static char big_time[] = "00:00";
    static char seconds_time[] = ":00.0";
    time_t display_time = 0;

    // if stop watch show elapsed else is in countdown mode
    if(isstopwatch)
    {
	display_time = elapsed_time;
    }
    else
    {
	display_time = (elapsed_time > start_gun_time) ? 0:(start_gun_time - elapsed_time);
	if(display_time == 0)
	{
		stop_stopwatch();
		vibes_enqueue_custom_pattern(start_pattern);
	}
    }


    // Now convert to hours/minutes/seconds.
    int hundredths = (display_time / 100) % 10;
    int seconds = (display_time / 1000) % 60;
    int minutes = (display_time / 60000) % 60;
    int hours = display_time / 3600000;

    // We can't fit three digit hours, so stop timing here.
    if(hours > 99) {
        stop_stopwatch();
        return;
    }

    itoa2(hours, &big_time[0]);
    itoa2(minutes, &big_time[3]);
    itoa2(seconds, &seconds_time[1]);
    itoa1(hundredths, &seconds_time[4]);

    // Now draw the strings.
    text_layer_set_text(&big_time_layer, big_time);
    text_layer_set_text(&seconds_time_layer, seconds_time);
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
    // First we need to generate a string.
    int hundredths = (lap_time / 100) % 10;
    int seconds = (lap_time / 1000) % 60;
    int minutes = (lap_time / 60000) % 60;
    int hours = lap_time / 3600000;
    // Fix up our buffer
    itoa2(hours, &lap_times[next_lap_layer][0]);
    itoa2(minutes, &lap_times[next_lap_layer][3]);
    itoa2(seconds, &lap_times[next_lap_layer][6]);
    itoa1(hundredths, &lap_times[next_lap_layer][9]);

    // Animate it
    static PropertyAnimation entry_animation;
    static GRect origin; origin = GRect(-139, 52, 139, 26);
    static GRect target; target = GRect(5, 52, 139, 26);
    property_animation_init_layer_frame(&entry_animation, &lap_layers[next_lap_layer].layer, &origin, &target);
    animation_set_curve(&entry_animation.animation, AnimationCurveEaseOut);
    animation_set_delay(&entry_animation.animation, 50);
    animation_set_handlers(&entry_animation.animation, (AnimationHandlers){
        .stopped = (AnimationStoppedHandler)animation_stopped
    }, NULL);
    animation_schedule(&entry_animation.animation);
    next_lap_layer = (next_lap_layer + 1) % LAP_TIME_SIZE;

}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
    (void)handle;
    if(cookie == TIMER_UPDATE) {
        if(started) {
            elapsed_time += 100;
            update_timer = app_timer_send_event(ctx, 100, TIMER_UPDATE);
	    time_t pebble_time = get_pebble_time();
	    if(!last_pebble_time) last_pebble_time = pebble_time;
	    if(pebble_time > last_pebble_time) {
                // If it's the first tick, instead of changing our time we calculate the correct time.
                if(!start_time) {
                    start_time = pebble_time - elapsed_time;
                } else {
                    elapsed_time = pebble_time - start_time;
                }
                last_pebble_time = pebble_time;
            }
        }
        update_stopwatch();
    }
}

// Toggle stopwatch timer mode
void toggle_mode(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  isstopwatch = isstopwatch ? false:true;

}

void config_provider(ClickConfig **config, Window *window) {
    config[BUTTON_ID_SELECT]->click.handler = (ClickHandler)toggle_stopwatch_handler;
    config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) toggle_mode;
    config[BUTTON_ID_DOWN]->click.handler = (ClickHandler)reset_stopwatch_handler;
    config[BUTTON_ID_UP]->click.handler = (ClickHandler)lap_time_handler;
    (void)window;
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .timer_handler = &handle_timer
  };
  app_event_loop(params, &handlers);
}
