
#include "pebble_os.h"
#include "pebbleme.h"
#include <time.h>
#include <sys/select.h>
#include <ctype.h>
#include <curses.h>
#include <math.h>
#include <errno.h>

static AppContext *theContext;

// The list of providers.
ClickConfig but_back, but_up, but_select, but_down;
ClickConfig *buttons[NUM_BUTTONS];

void click_config_init()
{
    buttons[BUTTON_ID_BACK]= &but_back;
    buttons[BUTTON_ID_DOWN] = &but_down;
    buttons[BUTTON_ID_SELECT] = &but_select;
    buttons[BUTTON_ID_UP] = &but_up;
    
    for(int i=0;i<NUM_BUTTONS;i++)
    {
        buttons[i]->click.handler=NULL;
        buttons[i]->click.repeat_interval_ms=0;
        buttons[i]->long_click.handler=NULL;
        buttons[i]->long_click.delay_ms=0;
        buttons[i]->long_click.release_handler=NULL;
        buttons[i]->multi_click.handler=NULL;
        buttons[i]->multi_click.last_click_only=true;
        buttons[i]->multi_click.max=0;
        buttons[i]->multi_click.min=0;
        buttons[i]->multi_click.timeout=0;
        buttons[i]->context=NULL;
        buttons[i]->raw.context=NULL;
        buttons[i]->raw.down_handler=NULL;
        buttons[i]->raw.up_handler=NULL;
    }
}
void appcontext_init( AppContext *app_ctx)
{
    app_ctx->activeTimers=0;
    app_ctx->topWindow = NULL;
    
	for(int i=0;i<MAXTIMERS;i++)
	{
		app_ctx->timeEvents[i].active = false;
		app_ctx->timeEvents[i].handle = 0;
		app_ctx->timeEvents[i].millis = 0;
		app_ctx->timeEvents[i].cookie = 0;
	}
    app_ctx->topWindow=NULL;
    app_ctx->theContext.fill_color=GColorBlack;
    app_ctx->theContext.stroke_color=GColorBlack;
    app_ctx->theContext.text_color=GColorBlack;
    
    // Save this to allow window to have basis of stack
    theContext = app_ctx;
	
}
void animation_init(struct Animation *animation){
	printf("Start:void animation_init(struct Animation *animation);\n");
	printf("end:void animation_init(struct Animation *animation);\n");
}

void animation_set_delay(struct Animation *animation, uint32_t delay_ms){
	printf("Start:void animation_set_delay(struct Animation *animation, uint32_t delay_ms);\n");
	printf("end:void animation_set_delay(struct Animation *animation, uint32_t delay_ms);\n");
}

void animation_set_duration(struct Animation *animation, uint32_t duration_ms){
	printf("Start:void animation_set_duration(struct Animation *animation, uint32_t duration_ms);\n");
	printf("end:void animation_set_duration(struct Animation *animation, uint32_t duration_ms);\n");
}

void animation_set_curve(struct Animation *animation, AnimationCurve curve){
	printf("Start:void animation_set_curve(struct Animation *animation, AnimationCurve curve);\n");
	printf("end:void animation_set_curve(struct Animation *animation, AnimationCurve curve);\n");
}

void animation_set_handlers(struct Animation *animation, AnimationHandlers callbacks, void *context){
	printf("Start:void animation_set_handlers(struct Animation *animation, AnimationHandlers callbacks, void *context);\n");
	printf("end:void animation_set_handlers(struct Animation *animation, AnimationHandlers callbacks, void *context);\n");
}

void animation_set_implementation(struct Animation *animation, const AnimationImplementation *implementation){
	printf("Start:void animation_set_implementation(struct Animation *animation, const AnimationImplementation *implementation);\n");
	printf("end:void animation_set_implementation(struct Animation *animation, const AnimationImplementation *implementation);\n");
}

void *animation_get_context(struct Animation *animation){
	printf("Start:void *animation_get_context(struct Animation *animation);\n");
	printf("end:void *animation_get_context(struct Animation *animation);\n");
    return(animation->context);
}

void animation_schedule(struct Animation *animation){
	printf("Start:void animation_schedule(struct Animation *animation);\n");
	printf("end:void animation_schedule(struct Animation *animation);\n");
}

void animation_unschedule(struct Animation *animation){
	printf("Start:void animation_unschedule(struct Animation *animation);\n");
	printf("end:void animation_unschedule(struct Animation *animation);\n");
}

void animation_unschedule_all(void){
	printf("Start:void animation_unschedule_all(void);\n");
	printf("end:void animation_unschedule_all(void);\n");
}

bool animation_is_scheduled(struct Animation *animation){
	printf("Start:bool animation_is_scheduled(struct Animation *animation);\n");
	printf("end:bool animation_is_scheduled(struct Animation *animation);\n");
    return(!animation->is_completed);
}

AppTimerHandle app_timer_send_event(AppContextRef app_ctx, uint32_t timeout_ms, uint32_t cookie){
	printf("Start:AppTimerHandle app_timer_send_event(AppContextRef app_ctx, uint32_t timeout_ms, uint32_t cookie);\n");
    AppContext *theContext = (AppContext *)app_ctx;
	AppTimerHandle handle= -1;

	for(int i=0;i<MAXTIMERS;i++)
	{
		if(!(theContext->timeEvents[i].active))
		{
			handle = i;
			theContext->timeEvents[i].active = true;
			theContext->timeEvents[i].handle = handle;
			theContext->timeEvents[i].millis = timeout_ms;
			theContext->timeEvents[i].cookie = cookie;
			break;
		}
	}
	printf("TimeofEvent: %d, cookie:%d, handle:%d \n",timeout_ms, cookie,handle);
	
	printf("end:AppTimerHandle app_timer_send_event(AppContextRef app_ctx, uint32_t timeout_ms, uint32_t cookie);\n");
	return(handle);
}

bool app_timer_cancel_event(AppContextRef app_ctx_ref, AppTimerHandle handle){
    
	printf("Start:bool app_timer_cancel_event(AppContextRef app_ctx_ref, AppTimerHandle handle:%d);\n",handle);
    AppContext *theContext = (AppContext *)app_ctx_ref;
	bool found = false;
	for(int i=0;i<MAXTIMERS;i++)
        {
                if((theContext->timeEvents[i].active) && theContext->timeEvents[i].handle==handle)
                {
                    found = true;
                    theContext->timeEvents[i].active = false;
                    theContext->timeEvents[i].handle = 0;
                    theContext->timeEvents[i].millis = 0;
                    theContext->timeEvents[i].cookie = 0;
                    break;
                }
        }
	printf("end:bool app_timer_cancel_event(AppContextRef app_ctx_ref, AppTimerHandle handle); return=%d\n",found);
	return (found);
}

void app_event_loop(AppTaskContextRef app_task_ctx, PebbleAppHandlers *handlers){
	printf("Start:void app_event_loop(AppTaskContextRef app_task_ctx, PebbleAppHandlers *handlers);\n");
    AppContext *theContext = (AppContext *)app_task_ctx;
    handlers->init_handler(theContext);
    struct fd_set readfds;
    struct timeval wait;
    char input='G';
    int selret=0,ret=0;
    // Loop until back hit
    do {
        FD_ZERO(&readfds);
        FD_SET(fileno(stdin),&readfds);
        wait.tv_sec = 0;
        // Always tick every second for the minute
        wait.tv_usec = 100000L;
        //
        selret= select(5,&readfds,NULL,NULL,&wait);
        ret=selret;
        switch(ret)
        {
                // Timeout
            case 0:
                for(int i=0;i<MAXTIMERS;i++)
                {
                    if(theContext->timeEvents[i].active)
                    {
                        theContext->timeEvents[i].active=false;
                        handlers->timer_handler((AppContextRef)theContext,theContext->timeEvents[i].handle,theContext->timeEvents[i].cookie);
                    }
                }
                
                break;
                // Error
            case -1:
                printf("Errno:%d\n",errno);
                break;
                // Some data
            default:
                if (FD_ISSET(fileno(stdin), &readfds)){
                    input = getchar();
                    input = toupper(input);
                    click_config_init();
                    if(theContext->topWindow->click_config_provider)
                    {
                        theContext->topWindow->click_config_provider(buttons,theContext->topWindow);
                    }
                    // See if we have any click handling
                    switch(input)
                    {
                            // Up Short
                            case 'U':
                                if(buttons[BUTTON_ID_UP]->click.handler)
                                    buttons[BUTTON_ID_UP]->click.handler(NULL, theContext->topWindow);
                            break;
                            // Down short
                            case 'D':
                                if(buttons[BUTTON_ID_DOWN]->click.handler)
                                    buttons[BUTTON_ID_DOWN]->click.handler(NULL, theContext->topWindow);
                                break;
                            case 'L':
                                if(buttons[BUTTON_ID_SELECT]->click.handler)
                                    buttons[BUTTON_ID_SELECT]->click.handler(NULL, theContext->topWindow);
                                break;
                            // Up long
                            case 'Y':
                                if(buttons[BUTTON_ID_UP]->long_click.handler)
                                    buttons[BUTTON_ID_UP]->long_click.handler(NULL, theContext->topWindow);
                                break;
                            // Down long
                            case 'S':
                                if(buttons[BUTTON_ID_DOWN]->long_click.handler)
                                    buttons[BUTTON_ID_DOWN]->long_click.handler(NULL, theContext->topWindow);
                            break;
                            case 'K':
                                if(buttons[BUTTON_ID_SELECT]->long_click.handler)
                                    buttons[BUTTON_ID_SELECT]->long_click.handler(NULL, theContext->topWindow);
                            break;
                        
                    }
                }
                break;
        }
    } while ( input != 'Q');

    handlers->deinit_handler(theContext);
	printf("end:void app_event_loop(AppTaskContextRef app_task_ctx, PebbleAppHandlers *handlers);\n");
}

bool bmp_init_container(int resource_id, BmpContainer *c){
	printf("Start:bool bmp_init_container(int resource_id, BmpContainer *c);\n");
	printf("resourceid=%d\n",resource_id);
	c->data = resource_id;
	layer_init(&(c->layer.layer), GRectZero);
	printf("end:bool bmp_init_container(int resource_id, BmpContainer *c);\n");
	return(true);
}

void bmp_deinit_container(BmpContainer *c){
	printf("Start:void bmp_deinit_container(BmpContainer *c);\n");
	printf("end:void bmp_deinit_container(BmpContainer *c);\n");
}

int32_t cos_lookup(int32_t angle){
	printf("Start:int32_t cos_lookup(int32_t angle);\n");
	printf("end:int32_t cos_lookup(int32_t angle);\n");
}

GFont fonts_get_system_font(const char *font_key){
	printf("Start:GFont fonts_get_system_font(const char *font_key);%s\n",font_key);
	printf("end:GFont fonts_get_system_font(const char *font_key);\n");
}

GFont fonts_load_custom_font(ResHandle resource){
	printf("Start:GFont fonts_load_custom_font(ResHandle resource);\n");
	printf("end:GFont fonts_load_custom_font(ResHandle resource);\n");
}

void fonts_unload_custom_font(GFont font){
	printf("Start:void fonts_unload_custom_font(GFont font);\n");
	printf("end:void fonts_unload_custom_font(GFont font);\n");
}

void graphics_context_set_stroke_color(GContext *ctx, GColor color){
	printf("Start:void graphics_context_set_stroke_color(GContext *ctx, GColor color);\n");
    ctx->stroke_color = color;
	printf("end:void graphics_context_set_stroke_color(GContext *ctx, GColor color);\n");
}

void graphics_context_set_fill_color(GContext *ctx, GColor color){
	printf("Start:void graphics_context_set_fill_color(GContext *ctx, GColor color);\n");
    ctx->fill_color=color;
	printf("end:void graphics_context_set_fill_color(GContext *ctx, GColor color);\n");
}

void graphics_context_set_text_color(GContext *ctx, GColor color){
	printf("Start:void graphics_context_set_text_color(GContext *ctx, GColor color);\n");
    ctx->text_color=color;
	printf("end:void graphics_context_set_text_color(GContext *ctx, GColor color);\n");
}

void graphics_context_set_compositing_mode(GContext *ctx, GCompOp mode){
	printf("Start:void graphics_context_set_compositing_mode(GContext *ctx, GCompOp mode);\n");
    ctx->compositing_mode=mode;
	printf("end:void graphics_context_set_compositing_mode(GContext *ctx, GCompOp mode);\n");
}

void graphics_draw_pixel(GContext *ctx, GPoint point){
	printf("Start:void graphics_draw_pixel(GContext *ctx, GPoint point);\n");
	printf("end:void graphics_draw_pixel(GContext *ctx, GPoint point);\n");
}

void graphics_draw_line(GContext *ctx, GPoint p0, GPoint p1){
	printf("Start:void graphics_draw_line(GContext *ctx, GPoint p0, GPoint p1);\n");
	printf("end:void graphics_draw_line(GContext *ctx, GPoint p0, GPoint p1);\n");
}

void graphics_fill_rect(GContext *ctx, GRect rect, uint8_t corner_radius, GCornerMask corner_mask){
	printf("Start:void graphics_fill_rect(GContext *ctx, GRect rect, uint8_t corner_radius, GCornerMask corner_mask);\n");
	printf("end:void graphics_fill_rect(GContext *ctx, GRect rect, uint8_t corner_radius, GCornerMask corner_mask);\n");
}

void graphics_draw_circle(GContext *ctx, GPoint p, int radius){
	printf("Start:void graphics_draw_circle(GContext *ctx, GPoint p, int radius);\n");
	printf("end:void graphics_draw_circle(GContext *ctx, GPoint p, int radius);\n");
}

void graphics_fill_circle(GContext *ctx, GPoint p, int radius){
	printf("Start:void graphics_fill_circle(GContext *ctx, GPoint p, int radius);\n");
	printf("end:void graphics_fill_circle(GContext *ctx, GPoint p, int radius);\n");
}

void graphics_draw_round_rect(GContext *ctx, GRect rect, int radius){
	printf("Start:void graphics_draw_round_rect(GContext *ctx, GRect rect, int radius);\n");
	printf("end:void graphics_draw_round_rect(GContext *ctx, GRect rect, int radius);\n");
}

void get_time(PblTm *time2){
	printf("Start:void get_time(PblTm *time);\n");
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	mktime(timeinfo);
	time2->tm_sec = timeinfo->tm_sec;
	time2->tm_min = timeinfo->tm_min;
	time2->tm_hour = timeinfo->tm_hour;
	time2->tm_mday = timeinfo->tm_mday;
	time2->tm_mon = timeinfo->tm_mon;
	time2->tm_year = timeinfo->tm_year;
	time2->tm_wday = timeinfo->tm_wday;
	time2->tm_yday = timeinfo->tm_yday;
	time2->tm_isdst = timeinfo->tm_isdst;
	printf("end:void get_time(PblTm *time);\n");
}

void gpath_init(GPath *path, const GPathInfo *init){
	printf("Start:void gpath_init(GPath *path, const GPathInfo *init);\n");
	printf("end:void gpath_init(GPath *path, const GPathInfo *init);\n");
}

void gpath_move_to(GPath *path, GPoint point){
	printf("Start:void gpath_move_to(GPath *path, GPoint point);\n");
	printf("end:void gpath_move_to(GPath *path, GPoint point);\n");
}

void gpath_rotate_to(GPath *path, int32_t angle){
	printf("Start:void gpath_rotate_to(GPath *path, int32_t angle);\n");
	printf("end:void gpath_rotate_to(GPath *path, int32_t angle);\n");
}

void gpath_draw_outline(GContext *ctx, GPath *path){
	printf("Start:void gpath_draw_outline(GContext *ctx, GPath *path);\n");
	printf("end:void gpath_draw_outline(GContext *ctx, GPath *path);\n");
}

void gpath_draw_filled(GContext *ctx, GPath *path){
	printf("Start:void gpath_draw_filled(GContext *ctx, GPath *path);\n");
	printf("end:void gpath_draw_filled(GContext *ctx, GPath *path);\n");
}

GPoint grect_center_point(GRect *rect){
	printf("Start:GPoint grect_center_point(GRect *rect);\n");
	printf("end:GPoint grect_center_point(GRect *rect);\n");
    return(GPoint((rect->origin.x + rect->size.w/2),(rect->origin.y + rect->size.h/2)));
}

void layer_mark_dirty(Layer *layer){
	printf("Start:void layer_mark_dirty(Layer *layer);\n");
	printf("end:void layer_mark_dirty(Layer *layer);\n");
}

void layer_remove_from_parent(Layer *child){
	printf("Start:void layer_remove_from_parent(Layer *child);\n");
	Layer *iterator,*lastiterator;
	if(child->parent)
	{
		if(child->parent->first_child != child && child->parent->first_child)
		{
			lastiterator = child->parent->first_child;
			iterator = child->parent->first_child->next_sibling;
			while(iterator != child && iterator)
			{
				lastiterator = iterator;
				iterator = iterator->next_sibling;
			}
            if(iterator == child)
            {
                lastiterator->next_sibling = NULL;
            }
		}
		else
			child->parent->first_child = NULL;
	}
    if(child)
        child->parent = NULL;
    else
        printf("Null pointer to layer_remove_from_parent\n");
	printf("end:void layer_remove_from_parent(Layer *child);\n");
}

void layer_add_child(Layer *parent, Layer *child){
	printf("Start:void layer_add_child(Layer *parent, Layer *child);\n");
	Layer *next=NULL;
	child->parent = parent;
	if(parent->first_child)
	{
		next = parent->first_child;
		while(next->next_sibling)
			next = next->next_sibling;
		next->next_sibling = child;
	}
	else
	{
		parent->first_child = child;
	}
	printf("end:void layer_add_child(Layer *parent, Layer *child);\n");
}

GRect layer_get_frame(Layer *layer){
	printf("Start:GRect layer_get_frame(Layer *layer);\n");
	printf("end:GRect layer_get_frame(Layer *layer);\n");
    return(layer->frame);
}

void layer_set_frame(Layer *layer, GRect frame){
	printf("Start:void layer_set_frame(Layer *layer, GRect frame);\n");
	layer->frame = frame;
	printf("end:void layer_set_frame(Layer *layer, GRect frame);\n");
}

void layer_set_hidden(Layer *layer, bool hidden){
	printf("Start:void layer_set_hidden(Layer *layer, bool hidden);\n");
	printf("mode:%s\n",(hidden?"Hide":"Unhide"));
	layer->hidden = hidden;
	printf("end:void layer_set_hidden(Layer *layer, bool hidden);\n");
}

void layer_init(Layer *layer, GRect frame){
	printf("Start:void layer_init(Layer *layer, GRect frame);\n");
	layer->frame = frame;
	layer->clips = true;
	layer->hidden = true;
	layer->parent = NULL;
	layer->next_sibling = NULL;
	layer->first_child = NULL;
	layer->window = NULL;
	layer->update_proc = NULL;
	printf("end:void layer_init(Layer *layer, GRect frame);\n");
}

void light_enable(bool enable){
	printf("Start:void light_enable(bool enable);\n");
	printf("end:void light_enable(bool enable);\n");
}

void light_enable_interaction(void){
	printf("Start:void light_enable_interaction(void);\n");
	printf("end:void light_enable_interaction(void);\n");
}

void psleep(int millis){
	printf("Start:void psleep(int millis);\n");
	printf("end:void psleep(int millis);\n");
}

void resource_init_current_app(ResVersionHandle version){
	printf("Start:void resource_init_current_app(ResVersionHandle version);\n");
	printf("end:void resource_init_current_app(ResVersionHandle version);\n");
}

ResHandle resource_get_handle(uint32_t file_id){
	printf("Start:ResHandle resource_get_handle(uint32_t file_id);\n");
	printf("end:ResHandle resource_get_handle(uint32_t file_id);\n");
}

size_t resource_load(ResHandle h, uint8_t *buffer, size_t max_length){
	printf("Start:size_t resource_load(ResHandle h, uint8_t *buffer, size_t max_length);\n");
	printf("end:size_t resource_load(ResHandle h, uint8_t *buffer, size_t max_length);\n");
}

size_t resource_load_byte_range(ResHandle h, uint32_t start_bytes, uint8_t *data, size_t num_bytes){
	printf("Start:size_t resource_load_byte_range(ResHandle h, uint32_t start_bytes, uint8_t *data, size_t num_bytes);\n");
	printf("end:size_t resource_load_byte_range(ResHandle h, uint32_t start_bytes, uint8_t *data, size_t num_bytes);\n");
}

size_t resource_size(ResHandle h){
	printf("Start:size_t resource_size(ResHandle h);\n");
	printf("end:size_t resource_size(ResHandle h);\n");
}

void rotbmp_deinit_container(RotBmpContainer *c){
	printf("Start:void rotbmp_deinit_container(RotBmpContainer *c);\n");
	printf("end:void rotbmp_deinit_container(RotBmpContainer *c);\n");
}

bool rotbmp_init_container(int resource_id, RotBmpContainer *c){
	printf("Start:bool rotbmp_init_container(int resource_id, RotBmpContainer *c);\n");
	printf("end:bool rotbmp_init_container(int resource_id, RotBmpContainer *c);\n");
}

void rotbmp_pair_deinit_container(RotBmpPairContainer *c){
	printf("Start:void rotbmp_pair_deinit_container(RotBmpPairContainer *c);\n");
	printf("end:void rotbmp_pair_deinit_container(RotBmpPairContainer *c);\n");
}

bool rotbmp_pair_init_container(int white_resource_id, int black_resource_id, RotBmpPairContainer *c){
	printf("Start:bool rotbmp_pair_init_container(int white_resource_id, int black_resource_id, RotBmpPairContainer *c);\n");
	printf("end:bool rotbmp_pair_init_container(int white_resource_id, int black_resource_id, RotBmpPairContainer *c);\n");
	return(true);
}

void rotbmp_pair_layer_set_src_ic(RotBmpPairLayer *pair, GPoint ic){
	printf("Start:void rotbmp_pair_layer_set_src_ic(RotBmpPairLayer *pair, GPoint ic);\n");
	printf("end:void rotbmp_pair_layer_set_src_ic(RotBmpPairLayer *pair, GPoint ic);\n");
}

void rotbmp_pair_layer_set_angle(RotBmpPairLayer *pair, int32_t angle){
	printf("Start:void rotbmp_pair_layer_set_angle(RotBmpPairLayer *pair, int32_t angle);\n");
	printf("end:void rotbmp_pair_layer_set_angle(RotBmpPairLayer *pair, int32_t angle);\n");
}

void window_init(Window *window, const char *debug_name){
	printf("Start:void window_init(Window *window, const char *debug_name);\n");
    window->debug_name=debug_name;
    window->background_color = GColorBlack;
    window->click_config_provider=NULL;
    window->is_fullscreen=false;
    window->is_loaded=false;
    window->is_render_scheduled=false;
    layer_init( &(window->layer), GRectZero) ;
    window->on_screen=false;
    window->overrides_back_button=false;
    window->status_bar_icon=NULL;
    window->user_data=NULL;
    // Save window in layer stack
    window->layer.window = NULL;
	printf("end:void window_init(Window *window, const char *debug_name);\n");
}

void window_stack_push(Window *window, bool animated){
	printf("Start:void window_stack_push(Window *window, bool animated);\n");
    // Already have a window on stack so push it down an dreplace with this.
    if(theContext->topWindow)
    {
        window->layer.window = theContext->topWindow;
    }
    theContext->topWindow = window;
	printf("end:void window_stack_push(Window *window, bool animated);\n");
}

void window_set_click_config_provider(Window *window, ClickConfigProvider click_config_provider){
	printf("Start:void window_set_click_config_provider(Window *window, ClickConfigProvider click_config_provider);\n");
    window->click_config_provider=click_config_provider;
	printf("end:void window_set_click_config_provider(Window *window, ClickConfigProvider click_config_provider);\n");
}

void window_set_background_color(Window *window, GColor background_color){
	printf("Start:void window_set_background_color(Window *window, GColor background_color);\n");
	printf("end:void window_set_background_color(Window *window, GColor background_color);\n");
}

void window_render(Window *window, GContext *ctx){
	printf("Start:void window_render(Window *window, GContext *ctx);\n");
	printf("end:void window_render(Window *window, GContext *ctx);\n");
}

void window_set_fullscreen(Window *window, bool enabled){
	printf("Start:void window_set_fullscreen(Window *window, bool enabled);\n");
	printf("end:void window_set_fullscreen(Window *window, bool enabled);\n");
}

int32_t sin_lookup(int32_t angle){
	printf("Start:int32_t sin_lookup(int32_t angle);\n");
	printf("end:int32_t sin_lookup(int32_t angle);\n");
	return(1);
}

void string_format_time(char *ptr, size_t maxsize, const char *format, const PblTm *timeptr){
	printf("Start:void string_format_time(char *ptr, size_t maxsize, const char *format, const PblTm *timeptr);\n");
    struct tm time;
    time.tm_hour = timeptr->tm_hour;
    time.tm_isdst = timeptr->tm_isdst;
    time.tm_mday = timeptr->tm_mday;
    time.tm_min = timeptr->tm_min;
    time.tm_mon = timeptr->tm_mon;
    time.tm_sec = timeptr->tm_sec;
    time.tm_wday = timeptr->tm_wday;
    time.tm_yday = timeptr->tm_yday;
    time.tm_year = timeptr->tm_year;
    time.tm_zone = "GMT0BST";
    time.tm_gmtoff = 0;
    strftime(ptr, maxsize, format, &time);
	printf("end:void string_format_time(char *ptr, size_t maxsize, const char *format, const PblTm *timeptr);\n");
}

void text_layer_init(TextLayer *text_layer, GRect frame){
	printf("Start:void text_layer_init(TextLayer *text_layer, GRect frame);\n");
    text_layer->layer.frame = frame;
	printf("end:void text_layer_init(TextLayer *text_layer, GRect frame);\n");
}

const char *text_layer_get_text(TextLayer *text_layer){
	printf("Start:const char *text_layer_get_text(TextLayer *text_layer);\n");
    
	printf("end:const char *text_layer_get_text(TextLayer *text_layer);\n");
    return(text_layer->text);
}

void text_layer_set_text(TextLayer *text_layer, const char *text){
	printf("Start:void text_layer_set_text(TextLayer *text_layer, const char *text);\n");
    text_layer->text = text;
	printf("end:void text_layer_set_text(TextLayer *text_layer, const char *text);%s\n", text_layer->text);
}

void text_layer_set_font(TextLayer *text_layer, GFont font){
	printf("Start:void text_layer_set_font(TextLayer *text_layer, GFont font);\n");
    text_layer->font = font;
	printf("end:void text_layer_set_font(TextLayer *text_layer, GFont font);\n");
}

void text_layer_set_text_color(TextLayer *text_layer, GColor color){
	printf("Start:void text_layer_set_text_color(TextLayer *text_layer, GColor color);\n");
    text_layer->text_color = color;
	printf("end:void text_layer_set_text_color(TextLayer *text_layer, GColor color);\n");
}

void text_layer_set_background_color(TextLayer *text_layer, GColor color){
	printf("Start:void text_layer_set_background_color(TextLayer *text_layer, GColor color);\n");
    text_layer->background_color = color;
	printf("end:void text_layer_set_background_color(TextLayer *text_layer, GColor color);\n");
}

void vibes_double_pulse(void) {
	printf("Start:void vibes_double_pulse(void);\n");
	printf("end:void vibes_double_pulse(void);\n");
}

void vibes_enqueue_custom_pattern(VibePattern pattern){
	printf("Start:void vibes_enqueue_custom_pattern(VibePattern pattern);\n");
	printf("end:void vibes_enqueue_custom_pattern(VibePattern pattern);\n");
}

void vibes_long_pulse(void){
	printf("Start:void vibes_long_pulse(void);\n");
	printf("end:void vibes_long_pulse(void);\n");
}

void vibes_short_pulse(void){
	printf("Start:void vibes_short_pulse(void);\n");
	printf("end:void vibes_short_pulse(void);\n");
}

GContext *app_get_current_graphics_context(void){
	printf("Start:GContext *app_get_current_graphics_context(void);\n");
	printf("end:GContext *app_get_current_graphics_context(void);\n");
    return((GContext *)&(theContext->theContext));
}

bool clock_is_24h_style(void){
	printf("Start:bool clock_is_24h_style(void);\n");
	printf("end:bool clock_is_24h_style(void);\n");
	return(true);
}

void property_animation_init_layer_frame(struct PropertyAnimation *property_animation, struct Layer *layer, GRect *from_frame, GRect *to_frame){
	printf("Start:void property_animation_init_layer_frame(struct PropertyAnimation *property_animation, struct Layer *layer, GRect *from_frame, GRect *to_frame);\n");
	printf("end:void property_animation_init_layer_frame(struct PropertyAnimation *property_animation, struct Layer *layer, GRect *from_frame, GRect *to_frame);\n");
}

void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment text_alignment){
	printf("Start:void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment text_alignment);\n");
    text_layer->text_alignment = text_alignment;
	printf("end:void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment text_alignment);\n");
}

void graphics_draw_bitmap_in_rect(GContext *ctx, const GBitmap *bitmap, GRect rect){
	printf("Start:void graphics_draw_bitmap_in_rect(GContext *ctx, const GBitmap *bitmap, GRect rect);\n");
	printf("end:void graphics_draw_bitmap_in_rect(GContext *ctx, const GBitmap *bitmap, GRect rect);\n");
}

void graphics_text_draw(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, const GTextLayoutCacheRef layout){
	printf("Start:void graphics_text_draw(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, const GTextLayoutCacheRef layout);\n");
	printf("end:void graphics_text_draw(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, const GTextLayoutCacheRef layout);\n");
}

void layer_set_bounds(Layer *layer, GRect bounds){
	printf("Start:void layer_set_bounds(Layer *layer, GRect bounds);\n");
    layer->bounds = bounds;
	printf("end:void layer_set_bounds(Layer *layer, GRect bounds);\n");
}

GRect layer_get_bounds(Layer *layer){
	printf("Start:GRect layer_get_bounds(Layer *layer);\n");
	printf("end:GRect layer_get_bounds(Layer *layer);\n");
    return(layer->bounds);
}

void layer_set_update_proc(Layer *layer, LayerUpdateProc update_proc){
	printf("Start:void layer_set_update_proc(Layer *layer, LayerUpdateProc update_proc);\n");
	printf("end:void layer_set_update_proc(Layer *layer, LayerUpdateProc update_proc);\n");
    layer->update_proc = update_proc;
}

struct Window *layer_get_window(Layer *layer){
	printf("Start:struct Window *layer_get_window(Layer *layer);\n");
	printf("end:struct Window *layer_get_window(Layer *layer);\n");
    return(layer->window);
}

void layer_remove_child_layers(Layer *parent){
	printf("Start:void layer_remove_child_layers(Layer *parent);\n");
	printf("end:void layer_remove_child_layers(Layer *parent);\n");
}

void layer_insert_below_sibling(Layer *layer_to_insert, Layer *below_sibling_layer){
	printf("Start:void layer_insert_below_sibling(Layer *layer_to_insert, Layer *below_sibling_layer);\n");
	printf("end:void layer_insert_below_sibling(Layer *layer_to_insert, Layer *below_sibling_layer);\n");
}

void layer_insert_above_sibling(Layer *layer_to_insert, Layer *above_sibling_layer){
	printf("Start:void layer_insert_above_sibling(Layer *layer_to_insert, Layer *above_sibling_layer);\n");
	printf("end:void layer_insert_above_sibling(Layer *layer_to_insert, Layer *above_sibling_layer);\n");
}
bool layer_get_hidden(Layer *layer){
	printf("Start:bool layer_get_hidden(Layer *layer);\n");
	printf("end:bool layer_get_hidden(Layer *layer);\n");
    return(layer->hidden);
}

void layer_set_clips(Layer *layer, bool clips){
	printf("Start:void layer_set_clips(Layer *layer, bool clips);\n");
	printf("end:void layer_set_clips(Layer *layer, bool clips);\n");
    layer->clips=clips;
}

bool layer_get_clips(Layer *layer){
	printf("Start:bool layer_get_clips(Layer *layer);\n");
	printf("end:bool layer_get_clips(Layer *layer);\n");
    return(layer->clips);
}

GSize text_layer_get_max_used_size(GContext *ctx, TextLayer *text_layer){
	printf("Start:GSize text_layer_get_max_used_size(GContext *ctx, TextLayer *text_layer);\n");
	printf("end:GSize text_layer_get_max_used_size(GContext *ctx, TextLayer *text_layer);\n");
}

void text_layer_set_size(TextLayer *text_layer, const GSize max_size){
	printf("Start:void text_layer_set_size(TextLayer *text_layer, const GSize max_size);\n");
	printf("end:void text_layer_set_size(TextLayer *text_layer, const GSize max_size);\n");
}

void text_layer_set_overflow_mode(TextLayer *text_layer, GTextOverflowMode line_mode){
	printf("Start:void text_layer_set_overflow_mode(TextLayer *text_layer, GTextOverflowMode line_mode);\n");
	printf("end:void text_layer_set_overflow_mode(TextLayer *text_layer, GTextOverflowMode line_mode);\n");
}

GSize graphics_text_layout_get_max_used_size(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, GTextLayoutCacheRef layout){
	printf("Start:GSize graphics_text_layout_get_max_used_size(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, GTextLayoutCacheRef layout);\n");
	printf("end:GSize graphics_text_layout_get_max_used_size(GContext *ctx, const char *text, const GFont font, const GRect box, const GTextOverflowMode overflow_mode, const GTextAlignment alignment, GTextLayoutCacheRef layout);\n");
}

void inverter_layer_init(InverterLayer *inverter, GRect frame){
	printf("Start:void inverter_layer_init(InverterLayer *inverter, GRect frame);\n");
	printf("end:void inverter_layer_init(InverterLayer *inverter, GRect frame);\n");
}

void bitmap_layer_init(BitmapLayer *image, GRect frame){
	printf("Start:void bitmap_layer_init(BitmapLayer *image, GRect frame);\n");
	printf("end:void bitmap_layer_init(BitmapLayer *image, GRect frame);\n");
}

void bitmap_layer_set_bitmap(BitmapLayer *image, const GBitmap *bitmap){
	printf("Start:void bitmap_layer_set_bitmap(BitmapLayer *image, const GBitmap *bitmap);\n");
	printf("end:void bitmap_layer_set_bitmap(BitmapLayer *image, const GBitmap *bitmap);\n");
}

void bitmap_layer_set_alignment(BitmapLayer *image, GAlign alignment){
	printf("Start:void bitmap_layer_set_alignment(BitmapLayer *image, GAlign alignment);\n");
	printf("end:void bitmap_layer_set_alignment(BitmapLayer *image, GAlign alignment);\n");
}

void bitmap_layer_set_background_color(BitmapLayer *image, GColor color){
	printf("Start:void bitmap_layer_set_background_color(BitmapLayer *image, GColor color);\n");
	printf("end:void bitmap_layer_set_background_color(BitmapLayer *image, GColor color);\n");
}

void bitmap_layer_set_compositing_mode(BitmapLayer *image, GCompOp mode){
	printf("Start:void bitmap_layer_set_compositing_mode(BitmapLayer *image, GCompOp mode);\n");
	printf("end:void bitmap_layer_set_compositing_mode(BitmapLayer *image, GCompOp mode);\n");
}

bool heap_bitmap_init(HeapBitmap *hb, int resource_id){
	printf("Start:bool heap_bitmap_init(HeapBitmap *hb, int resource_id);\n");
	printf("end:bool heap_bitmap_init(HeapBitmap *hb, int resource_id);\n");
}

void heap_bitmap_deinit(HeapBitmap *hb){
	printf("Start:void heap_bitmap_deinit(HeapBitmap *hb);\n");
	printf("end:void heap_bitmap_deinit(HeapBitmap *hb);\n");
}

ButtonId click_recognizer_get_button_id(ClickRecognizerRef recognizer){
	printf("Start:ButtonId click_recognizer_get_button_id(ClickRecognizerRef recognizer);\n");
	printf("end:ButtonId click_recognizer_get_button_id(ClickRecognizerRef recognizer);\n");
}

uint8_t click_number_of_clicks_counted(ClickRecognizerRef recognizer){
	printf("Start:uint8_t click_number_of_clicks_counted(ClickRecognizerRef recognizer);\n");
	printf("end:uint8_t click_number_of_clicks_counted(ClickRecognizerRef recognizer);\n");
}

void menu_cell_basic_draw(GContext *ctx, Layer *cell_layer, const char *title, const char *subtitle, GBitmap *icon){
	printf("Start:void menu_cell_basic_draw(GContext *ctx, Layer *cell_layer, const char *title, const char *subtitle, GBitmap *icon);\n");
	printf("end:void menu_cell_basic_draw(GContext *ctx, Layer *cell_layer, const char *title, const char *subtitle, GBitmap *icon);\n");
}

void menu_cell_title_draw(GContext *ctx, Layer *cell_layer, const char *title){
	printf("Start:void menu_cell_title_draw(GContext *ctx, Layer *cell_layer, const char *title);\n");
	printf("end:void menu_cell_title_draw(GContext *ctx, Layer *cell_layer, const char *title);\n");
}

void menu_cell_basic_header_draw(GContext *ctx, Layer *cell_layer, const char *title){
	printf("Start:void menu_cell_basic_header_draw(GContext *ctx, Layer *cell_layer, const char *title);\n");
	printf("end:void menu_cell_basic_header_draw(GContext *ctx, Layer *cell_layer, const char *title);\n");
}

void menu_layer_init(MenuLayer *menu_layer, GRect frame){
	printf("Start:void menu_layer_init(MenuLayer *menu_layer, GRect frame);\n");
	printf("end:void menu_layer_init(MenuLayer *menu_layer, GRect frame);\n");
}

Layer *menu_layer_get_layer(MenuLayer *menu_layer){
	printf("Start:Layer *menu_layer_get_layer(MenuLayer *menu_layer);\n");
	printf("end:Layer *menu_layer_get_layer(MenuLayer *menu_layer);\n");
}

void menu_layer_set_callbacks(MenuLayer *menu_layer, void *callback_context, MenuLayerCallbacks callbacks){
	printf("Start:void menu_layer_set_callbacks(MenuLayer *menu_layer, void *callback_context, MenuLayerCallbacks callbacks);\n");
	printf("end:void menu_layer_set_callbacks(MenuLayer *menu_layer, void *callback_context, MenuLayerCallbacks callbacks);\n");
}

void menu_layer_set_click_config_onto_window(MenuLayer *menu_layer, struct Window *window){
	printf("Start:void menu_layer_set_click_config_onto_window(MenuLayer *menu_layer, struct Window *window);\n");
	printf("end:void menu_layer_set_click_config_onto_window(MenuLayer *menu_layer, struct Window *window);\n");
}

void menu_layer_set_selected_next(MenuLayer *menu_layer, bool up, MenuRowAlign scroll_align, bool animated){
	printf("Start:void menu_layer_set_selected_next(MenuLayer *menu_layer, bool up, MenuRowAlign scroll_align, bool animated);\n");
	printf("end:void menu_layer_set_selected_next(MenuLayer *menu_layer, bool up, MenuRowAlign scroll_align, bool animated);\n");
}

void menu_layer_set_selected_index(MenuLayer *menu_layer, MenuIndex index, MenuRowAlign scroll_align, bool animated){
	printf("Start:void menu_layer_set_selected_index(MenuLayer *menu_layer, MenuIndex index, MenuRowAlign scroll_align, bool animated);\n");
	printf("end:void menu_layer_set_selected_index(MenuLayer *menu_layer, MenuIndex index, MenuRowAlign scroll_align, bool animated);\n");
}

void menu_layer_reload_data(MenuLayer *menu_layer){
	printf("Start:void menu_layer_reload_data(MenuLayer *menu_layer);\n");
	printf("end:void menu_layer_reload_data(MenuLayer *menu_layer);\n");
}

int16_t menu_index_compare(MenuIndex *a, MenuIndex *b){
	printf("Start:int16_t menu_index_compare(MenuIndex *a, MenuIndex *b);\n");
	printf("end:int16_t menu_index_compare(MenuIndex *a, MenuIndex *b);\n");
}

void scroll_layer_init(ScrollLayer *scroll_layer, GRect frame){
	printf("Start:void scroll_layer_init(ScrollLayer *scroll_layer, GRect frame);\n");
	printf("end:void scroll_layer_init(ScrollLayer *scroll_layer, GRect frame);\n");
}

void scroll_layer_add_child(ScrollLayer *scroll_layer, Layer *child){
	printf("Start:void scroll_layer_add_child(ScrollLayer *scroll_layer, Layer *child);\n");
	printf("end:void scroll_layer_add_child(ScrollLayer *scroll_layer, Layer *child);\n");
}

void scroll_layer_set_click_config_onto_window(ScrollLayer *scroll_layer, struct Window *window){
	printf("Start:void scroll_layer_set_click_config_onto_window(ScrollLayer *scroll_layer, struct Window *window);\n");
	printf("end:void scroll_layer_set_click_config_onto_window(ScrollLayer *scroll_layer, struct Window *window);\n");
}

void scroll_layer_set_callbacks(ScrollLayer *scroll_layer, ScrollLayerCallbacks callbacks){
	printf("Start:void scroll_layer_set_callbacks(ScrollLayer *scroll_layer, ScrollLayerCallbacks callbacks);\n");
	printf("end:void scroll_layer_set_callbacks(ScrollLayer *scroll_layer, ScrollLayerCallbacks callbacks);\n");
}

void scroll_layer_set_context(ScrollLayer *scroll_layer, void *context){
	printf("Start:void scroll_layer_set_context(ScrollLayer *scroll_layer, void *context);\n");
	printf("end:void scroll_layer_set_context(ScrollLayer *scroll_layer, void *context);\n");
}

void scroll_layer_set_content_offset(ScrollLayer *scroll_layer, GPoint offset, bool animated){
	printf("Start:void scroll_layer_set_content_offset(ScrollLayer *scroll_layer, GPoint offset, bool animated);\n");
	printf("end:void scroll_layer_set_content_offset(ScrollLayer *scroll_layer, GPoint offset, bool animated);\n");
}

GPoint scroll_layer_get_content_offset(ScrollLayer *scroll_layer){
	printf("Start:GPoint scroll_layer_get_content_offset(ScrollLayer *scroll_layer);\n");
	printf("end:GPoint scroll_layer_get_content_offset(ScrollLayer *scroll_layer);\n");
}

void scroll_layer_set_content_size(ScrollLayer *scroll_layer, GSize size){
	printf("Start:void scroll_layer_set_content_size(ScrollLayer *scroll_layer, GSize size);\n");
	printf("end:void scroll_layer_set_content_size(ScrollLayer *scroll_layer, GSize size);\n");
}

GSize scroll_layer_get_content_size(ScrollLayer *scroll_layer){
	printf("Start:GSize scroll_layer_get_content_size(ScrollLayer *scroll_layer);\n");
	printf("end:GSize scroll_layer_get_content_size(ScrollLayer *scroll_layer);\n");
}

void scroll_layer_set_frame(ScrollLayer *scroll_layer, GRect rect){
	printf("Start:void scroll_layer_set_frame(ScrollLayer *scroll_layer, GRect rect);\n");
	printf("end:void scroll_layer_set_frame(ScrollLayer *scroll_layer, GRect rect);\n");
}

void scroll_layer_scroll_up_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer){
	printf("Start:void scroll_layer_scroll_up_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer);\n");
	printf("end:void scroll_layer_scroll_up_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer);\n");
}

void scroll_layer_scroll_down_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer){
	printf("Start:void scroll_layer_scroll_down_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer);\n");
	printf("end:void scroll_layer_scroll_down_click_handler(ClickRecognizerRef recognizer, ScrollLayer *scroll_layer);\n");
}

void simple_menu_layer_init(SimpleMenuLayer *simple_menu, GRect frame, Window *window, const SimpleMenuSection *sections, int num_sections, void *callback_context){
	printf("Start:void simple_menu_layer_init(SimpleMenuLayer *simple_menu, GRect frame, Window *window, const SimpleMenuSection *sections, int num_sections, void *callback_context);\n");
	printf("end:void simple_menu_layer_init(SimpleMenuLayer *simple_menu, GRect frame, Window *window, const SimpleMenuSection *sections, int num_sections, void *callback_context);\n");
}

Layer *simple_menu_layer_get_layer(SimpleMenuLayer *simple_menu){
	printf("Start:Layer *simple_menu_layer_get_layer(SimpleMenuLayer *simple_menu);\n");
	printf("end:Layer *simple_menu_layer_get_layer(SimpleMenuLayer *simple_menu);\n");
}

int simple_menu_layer_get_selected_index(SimpleMenuLayer *simple_menu){
	printf("Start:int simple_menu_layer_get_selected_index(SimpleMenuLayer *simple_menu);\n");
	printf("end:int simple_menu_layer_get_selected_index(SimpleMenuLayer *simple_menu);\n");
}

void simple_menu_layer_set_selected_index(SimpleMenuLayer *simple_menu, int index, bool animated){
	printf("Start:void simple_menu_layer_set_selected_index(SimpleMenuLayer *simple_menu, int index, bool animated);\n");
	printf("end:void simple_menu_layer_set_selected_index(SimpleMenuLayer *simple_menu, int index, bool animated);\n");
}

void window_deinit(Window *window){
	printf("Start:void window_deinit(Window *window);\n");
	printf("end:void window_deinit(Window *window);\n");
}

void window_set_click_config_provider_with_context(Window *window, ClickConfigProvider click_config_provider, void *context){
	printf("Start:void window_set_click_config_provider_with_context(Window *window, ClickConfigProvider click_config_provider, void *context);\n");
    window->click_config_provider=click_config_provider;
    window->click_config_context=context;
	printf("end:void window_set_click_config_provider_with_context(Window *window, ClickConfigProvider click_config_provider, void *context);\n");
}

ClickConfigProvider window_get_click_config_provider(Window *window){
	printf("Start:ClickConfigProvider window_get_click_config_provider(Window *window);\n");
	printf("end:ClickConfigProvider window_get_click_config_provider(Window *window);\n");
    return (window->click_config_provider);
}

void window_set_window_handlers(Window *window, WindowHandlers handlers){
	printf("Start:void window_set_window_handlers(Window *window, WindowHandlers handlers);\n");
    window->window_handlers = handlers;
	printf("end:void window_set_window_handlers(Window *window, WindowHandlers handlers);\n");
}

struct Layer *window_get_root_layer(Window *window){
	printf("Start:struct Layer *window_get_root_layer(Window *window);\n");
	
	printf("end:struct Layer *window_get_root_layer(Window *window);\n");
	return (&(window->layer));
}

bool window_get_fullscreen(Window *window){
	printf("Start:bool window_get_fullscreen(Window *window);\n");
	printf("end:bool window_get_fullscreen(Window *window);\n");
    return (window->is_fullscreen);
}

void window_set_status_bar_icon(Window *window, const GBitmap *icon){
	printf("Start:void window_set_status_bar_icon(Window *window, const GBitmap *icon);\n");
    window->status_bar_icon = icon;
	printf("end:void window_set_status_bar_icon(Window *window, const GBitmap *icon);\n");
}

bool window_is_loaded(Window *window){
	printf("Start:bool window_is_loaded(Window *window);\n");
	printf("end:bool window_is_loaded(Window *window);\n");
    return (window->is_loaded);
}

Window *window_stack_pop(bool animated){
	printf("Start:Window *window_stack_pop(bool animated);\n");
    Window *oldtop;
    oldtop = theContext->topWindow;
    theContext->topWindow = oldtop->layer.window;
    oldtop->layer.window=NULL;
	printf("end:Window *window_stack_pop(bool animated);\n");
    return(oldtop);
}

void window_stack_pop_all(const bool animated){
	printf("Start:void window_stack_pop_all(const bool animated);\n");
	printf("end:void window_stack_pop_all(const bool animated);\n");
}

bool window_stack_contains_window(Window *window){
	printf("Start:bool window_stack_contains_window(Window *window);\n");
	printf("end:bool window_stack_contains_window(Window *window);\n");
    bool retval = false;
    Window *top = theContext->topWindow;
    while(top)
    {
        if(top == window)
        {
            retval=true;
            break;
        }
        top = top->layer.window;
    }
    return(retval);
}

Window *window_stack_get_top_window(void){
	printf("Start:Window *window_stack_get_top_window(void);\n");
	printf("end:Window *window_stack_get_top_window(void);\n");
    return(theContext->topWindow);
}

Window *window_stack_remove(Window *window, bool animated){
	printf("Start:Window *window_stack_remove(Window *window, bool animated);\n");
    
	printf("end:Window *window_stack_remove(Window *window, bool animated);\n");
}

void property_animation_init(struct PropertyAnimation *property_animation, const struct PropertyAnimationImplementation *implementation, void *subject, void *from_value, void *to_value){
	printf("Start:void property_animation_init(struct PropertyAnimation *property_animation, const struct PropertyAnimationImplementation *implementation, void *subject, void *from_value, void *to_value);\n");
	printf("end:void property_animation_init(struct PropertyAnimation *property_animation, const struct PropertyAnimationImplementation *implementation, void *subject, void *from_value, void *to_value);\n");
}

void property_animation_update_int16(struct PropertyAnimation *property_animation, const uint32_t time_normalized){
	printf("Start:void property_animation_update_int16(struct PropertyAnimation *property_animation, const uint32_t time_normalized);\n");
	printf("end:void property_animation_update_int16(struct PropertyAnimation *property_animation, const uint32_t time_normalized);\n");
}

void property_animation_update_gpoint(struct PropertyAnimation *property_animation, const uint32_t time_normalized){
	printf("Start:void property_animation_update_gpoint(struct PropertyAnimation *property_animation, const uint32_t time_normalized);\n");
	printf("end:void property_animation_update_gpoint(struct PropertyAnimation *property_animation, const uint32_t time_normalized);\n");
}

void property_animation_update_grect(struct PropertyAnimation *property_animation, const uint32_t time_normalized){
	printf("Start:void property_animation_update_grect(struct PropertyAnimation *property_animation, const uint32_t time_normalized);\n");
	printf("end:void property_animation_update_grect(struct PropertyAnimation *property_animation, const uint32_t time_normalized);\n");
}

AppMessageResult app_message_register_callbacks(AppMessageCallbacksNode *callbacks_node){
	printf("Start:AppMessageResult app_message_register_callbacks(AppMessageCallbacksNode *callbacks_node);\n");
	printf("end:AppMessageResult app_message_register_callbacks(AppMessageCallbacksNode *callbacks_node);\n");
}

AppMessageResult app_message_deregister_callbacks(AppMessageCallbacksNode *callbacks_node){
	printf("Start:AppMessageResult app_message_deregister_callbacks(AppMessageCallbacksNode *callbacks_node);\n");
	printf("end:AppMessageResult app_message_deregister_callbacks(AppMessageCallbacksNode *callbacks_node);\n");
}

AppMessageResult app_message_out_get(DictionaryIterator **iter_out){
	printf("Start:AppMessageResult app_message_out_get(DictionaryIterator **iter_out);\n");
	printf("end:AppMessageResult app_message_out_get(DictionaryIterator **iter_out);\n");
}

AppMessageResult app_message_out_send(void){
	printf("Start:AppMessageResult app_message_out_send(void);\n");
	printf("end:AppMessageResult app_message_out_send(void);\n");
}

AppMessageResult app_message_out_release(void){
	printf("Start:AppMessageResult app_message_out_release(void);\n");
	printf("end:AppMessageResult app_message_out_release(void);\n");
}

void app_sync_init(AppSync *s, uint8_t *buffer, const uint16_t buffer_size, const Tuplet * const keys_and_initial_values, const uint8_t count, AppSyncTupleChangedCallback tuple_changed_callback, AppSyncErrorCallback error_callback, void *context){
	printf("Start:void app_sync_init(AppSync *s, uint8_t *buffer, const uint16_t buffer_size, const Tuplet * const keys_and_initial_values, const uint8_t count, AppSyncTupleChangedCallback tuple_changed_callback, AppSyncErrorCallback error_callback, void *context);\n");
	printf("end:void app_sync_init(AppSync *s, uint8_t *buffer, const uint16_t buffer_size, const Tuplet * const keys_and_initial_values, const uint8_t count, AppSyncTupleChangedCallback tuple_changed_callback, AppSyncErrorCallback error_callback, void *context);\n");
}

void app_sync_deinit(AppSync *s){
	printf("Start:void app_sync_deinit(AppSync *s);\n");
	printf("end:void app_sync_deinit(AppSync *s);\n");
}

AppMessageResult app_sync_set(AppSync *s, const Tuplet * const keys_and_values_to_update, const uint8_t count){
	printf("Start:AppMessageResult app_sync_set(AppSync *s, const Tuplet * const keys_and_values_to_update, const uint8_t count);\n");
	printf("end:AppMessageResult app_sync_set(AppSync *s, const Tuplet * const keys_and_values_to_update, const uint8_t count);\n");
}

const Tuple *app_sync_get(const AppSync *s, const uint32_t key){
	printf("Start:const Tuple *app_sync_get(const AppSync *s, const uint32_t key);\n");
	printf("end:const Tuple *app_sync_get(const AppSync *s, const uint32_t key);\n");
}

uint32_t dict_calc_buffer_size(const uint8_t tuple_count, ...){
	printf("Start:uint32_t dict_calc_buffer_size(const uint8_t tuple_count, ...);\n");
	printf("end:uint32_t dict_calc_buffer_size(const uint8_t tuple_count, ...);\n");
}

DictionaryResult dict_write_begin(DictionaryIterator *iter, uint8_t * const buffer, const uint16_t size){
	printf("Start:DictionaryResult dict_write_begin(DictionaryIterator *iter, uint8_t * const buffer, const uint16_t size);\n");
	printf("end:DictionaryResult dict_write_begin(DictionaryIterator *iter, uint8_t * const buffer, const uint16_t size);\n");
}

DictionaryResult dict_write_data(DictionaryIterator *iter, const uint32_t key, const uint8_t * const data, const uint16_t size){
	printf("Start:DictionaryResult dict_write_data(DictionaryIterator *iter, const uint32_t key, const uint8_t * const data, const uint16_t size);\n");
	printf("end:DictionaryResult dict_write_data(DictionaryIterator *iter, const uint32_t key, const uint8_t * const data, const uint16_t size);\n");
}

DictionaryResult dict_write_cstring(DictionaryIterator *iter, const uint32_t key, const char * const cstring){
	printf("Start:DictionaryResult dict_write_cstring(DictionaryIterator *iter, const uint32_t key, const char * const cstring);\n");
	printf("end:DictionaryResult dict_write_cstring(DictionaryIterator *iter, const uint32_t key, const char * const cstring);\n");
}

DictionaryResult dict_write_int(DictionaryIterator *iter, const uint32_t key, const void *integer, const uint8_t width_bytes, const bool is_signed){
	printf("Start:DictionaryResult dict_write_int(DictionaryIterator *iter, const uint32_t key, const void *integer, const uint8_t width_bytes, const bool is_signed);\n");
	printf("end:DictionaryResult dict_write_int(DictionaryIterator *iter, const uint32_t key, const void *integer, const uint8_t width_bytes, const bool is_signed);\n");
}

DictionaryResult dict_write_uint8(DictionaryIterator *iter, const uint32_t key, const uint8_t value){
	printf("Start:DictionaryResult dict_write_uint8(DictionaryIterator *iter, const uint32_t key, const uint8_t value);\n");
	printf("end:DictionaryResult dict_write_uint8(DictionaryIterator *iter, const uint32_t key, const uint8_t value);\n");
}

DictionaryResult dict_write_uint16(DictionaryIterator *iter, const uint32_t key, const uint16_t value){
	printf("Start:DictionaryResult dict_write_uint16(DictionaryIterator *iter, const uint32_t key, const uint16_t value);\n");
	printf("end:DictionaryResult dict_write_uint16(DictionaryIterator *iter, const uint32_t key, const uint16_t value);\n");
}

DictionaryResult dict_write_uint32(DictionaryIterator *iter, const uint32_t key, const uint32_t value){
	printf("Start:DictionaryResult dict_write_uint32(DictionaryIterator *iter, const uint32_t key, const uint32_t value);\n");
	printf("end:DictionaryResult dict_write_uint32(DictionaryIterator *iter, const uint32_t key, const uint32_t value);\n");
}

DictionaryResult dict_write_int8(DictionaryIterator *iter, const uint32_t key, const int8_t value){
	printf("Start:DictionaryResult dict_write_int8(DictionaryIterator *iter, const uint32_t key, const int8_t value);\n");
	printf("end:DictionaryResult dict_write_int8(DictionaryIterator *iter, const uint32_t key, const int8_t value);\n");
}

DictionaryResult dict_write_int16(DictionaryIterator *iter, const uint32_t key, const int16_t value){
	printf("Start:DictionaryResult dict_write_int16(DictionaryIterator *iter, const uint32_t key, const int16_t value);\n");
	printf("end:DictionaryResult dict_write_int16(DictionaryIterator *iter, const uint32_t key, const int16_t value);\n");
}

DictionaryResult dict_write_int32(DictionaryIterator *iter, const uint32_t key, const int32_t value){
	printf("Start:DictionaryResult dict_write_int32(DictionaryIterator *iter, const uint32_t key, const int32_t value);\n");
	printf("end:DictionaryResult dict_write_int32(DictionaryIterator *iter, const uint32_t key, const int32_t value);\n");
}

uint32_t dict_write_end(DictionaryIterator *iter){
	printf("Start:uint32_t dict_write_end(DictionaryIterator *iter);\n");
	printf("end:uint32_t dict_write_end(DictionaryIterator *iter);\n");
}

Tuple *dict_read_begin_from_buffer(DictionaryIterator *iter, const uint8_t * const buffer, const uint16_t size){
	printf("Start:Tuple *dict_read_begin_from_buffer(DictionaryIterator *iter, const uint8_t * const buffer, const uint16_t size);\n");
	printf("end:Tuple *dict_read_begin_from_buffer(DictionaryIterator *iter, const uint8_t * const buffer, const uint16_t size);\n");
}

Tuple *dict_read_next(DictionaryIterator *iter){
	printf("Start:Tuple *dict_read_next(DictionaryIterator *iter);\n");
	printf("end:Tuple *dict_read_next(DictionaryIterator *iter);\n");
}

Tuple *dict_read_first(DictionaryIterator *iter){
	printf("Start:Tuple *dict_read_first(DictionaryIterator *iter);\n");
	printf("end:Tuple *dict_read_first(DictionaryIterator *iter);\n");
}

DictionaryResult dict_serialize_tuplets(DictionarySerializeCallback callback, void *context, const uint8_t tuplets_count, const Tuplet * const tuplets){
	printf("Start:DictionaryResult dict_serialize_tuplets(DictionarySerializeCallback callback, void *context, const uint8_t tuplets_count, const Tuplet * const tuplets);\n");
	printf("end:DictionaryResult dict_serialize_tuplets(DictionarySerializeCallback callback, void *context, const uint8_t tuplets_count, const Tuplet * const tuplets);\n");
}

DictionaryResult dict_serialize_tuplets_to_buffer(const uint8_t tuplets_count, const Tuplet * const tuplets, uint8_t *buffer, uint32_t *size_in_out){
	printf("Start:DictionaryResult dict_serialize_tuplets_to_buffer(const uint8_t tuplets_count, const Tuplet * const tuplets, uint8_t *buffer, uint32_t *size_in_out);\n");
	printf("end:DictionaryResult dict_serialize_tuplets_to_buffer(const uint8_t tuplets_count, const Tuplet * const tuplets, uint8_t *buffer, uint32_t *size_in_out);\n");
}

DictionaryResult dict_write_tuplet(DictionaryIterator *iter, const Tuplet * const tuplet){
	printf("Start:DictionaryResult dict_write_tuplet(DictionaryIterator *iter, const Tuplet * const tuplet);\n");
	printf("end:DictionaryResult dict_write_tuplet(DictionaryIterator *iter, const Tuplet * const tuplet);\n");
}

uint32_t dict_calc_buffer_size_from_tuplets(const uint8_t tuplets_count, const Tuplet * const tuplets){
	printf("Start:uint32_t dict_calc_buffer_size_from_tuplets(const uint8_t tuplets_count, const Tuplet * const tuplets);\n");
	printf("end:uint32_t dict_calc_buffer_size_from_tuplets(const uint8_t tuplets_count, const Tuplet * const tuplets);\n");
}

DictionaryResult dict_merge(DictionaryIterator *dest, uint32_t *dest_max_size_in_out, DictionaryIterator *source, const bool update_existing_keys_only, const DictionaryKeyUpdatedCallback key_callback, void *context){
	printf("Start:DictionaryResult dict_merge(DictionaryIterator *dest, uint32_t *dest_max_size_in_out, DictionaryIterator *source, const bool update_existing_keys_only, const DictionaryKeyUpdatedCallback key_callback, void *context);\n");
	printf("end:DictionaryResult dict_merge(DictionaryIterator *dest, uint32_t *dest_max_size_in_out, DictionaryIterator *source, const bool update_existing_keys_only, const DictionaryKeyUpdatedCallback key_callback, void *context);\n");
}

Tuple *dict_find(const DictionaryIterator *iter, const uint32_t key){
	printf("Start:Tuple *dict_find(const DictionaryIterator *iter, const uint32_t key);\n");
	printf("end:Tuple *dict_find(const DictionaryIterator *iter, const uint32_t key);\n");
}

void action_bar_layer_init(ActionBarLayer *action_bar){
	printf("Start:void action_bar_layer_init(ActionBarLayer *action_bar);\n");
	printf("end:void action_bar_layer_init(ActionBarLayer *action_bar);\n");
}

void action_bar_layer_set_context(ActionBarLayer *action_bar, void *context){
	printf("Start:void action_bar_layer_set_context(ActionBarLayer *action_bar, void *context);\n");
	printf("end:void action_bar_layer_set_context(ActionBarLayer *action_bar, void *context);\n");
}

void action_bar_layer_set_click_config_provider(ActionBarLayer *action_bar, ClickConfigProvider click_config_provider){
	printf("Start:void action_bar_layer_set_click_config_provider(ActionBarLayer *action_bar, ClickConfigProvider click_config_provider);\n");
	printf("end:void action_bar_layer_set_click_config_provider(ActionBarLayer *action_bar, ClickConfigProvider click_config_provider);\n");
}

void action_bar_layer_set_icon(ActionBarLayer *action_bar, ButtonId button_id, const GBitmap *icon){
	printf("Start:void action_bar_layer_set_icon(ActionBarLayer *action_bar, ButtonId button_id, const GBitmap *icon);\n");
	printf("end:void action_bar_layer_set_icon(ActionBarLayer *action_bar, ButtonId button_id, const GBitmap *icon);\n");
}

void action_bar_layer_clear_icon(ActionBarLayer *action_bar, ButtonId button_id){
	printf("Start:void action_bar_layer_clear_icon(ActionBarLayer *action_bar, ButtonId button_id);\n");
	printf("end:void action_bar_layer_clear_icon(ActionBarLayer *action_bar, ButtonId button_id);\n");
}

void action_bar_layer_add_to_window(ActionBarLayer *action_bar, struct Window *window){
	printf("Start:void action_bar_layer_add_to_window(ActionBarLayer *action_bar, struct Window *window);\n");
	printf("end:void action_bar_layer_add_to_window(ActionBarLayer *action_bar, struct Window *window);\n");
}

void action_bar_layer_remove_from_window(ActionBarLayer *action_bar){
	printf("Start:void action_bar_layer_remove_from_window(ActionBarLayer *action_bar);\n");
	printf("end:void action_bar_layer_remove_from_window(ActionBarLayer *action_bar);\n");
}

void action_bar_layer_set_background_color(ActionBarLayer *action_bar, GColor background_color){
	printf("Start:void action_bar_layer_set_background_color(ActionBarLayer *action_bar, GColor background_color);\n");
	printf("end:void action_bar_layer_set_background_color(ActionBarLayer *action_bar, GColor background_color);\n");
}

void number_window_init(NumberWindow *numberwindow, const char *label, NumberWindowCallbacks callbacks, void *callback_context){
	printf("Start:void number_window_init(NumberWindow *numberwindow, const char *label, NumberWindowCallbacks callbacks, void *callback_context);\n");
	printf("end:void number_window_init(NumberWindow *numberwindow, const char *label, NumberWindowCallbacks callbacks, void *callback_context);\n");
}

void number_window_set_label(NumberWindow *nw, const char *label){
	printf("Start:void number_window_set_label(NumberWindow *nw, const char *label);\n");
	printf("end:void number_window_set_label(NumberWindow *nw, const char *label);\n");
}

void number_window_set_max(NumberWindow *numberwindow, int max){
	printf("Start:void number_window_set_max(NumberWindow *numberwindow, int max);\n");
	printf("end:void number_window_set_max(NumberWindow *numberwindow, int max);\n");
}

void number_window_set_min(NumberWindow *numberwindow, int min){
	printf("Start:void number_window_set_min(NumberWindow *numberwindow, int min);\n");
	printf("end:void number_window_set_min(NumberWindow *numberwindow, int min);\n");
}

void number_window_set_value(NumberWindow *numberwindow, int value){
	printf("Start:void number_window_set_value(NumberWindow *numberwindow, int value);\n");
	printf("end:void number_window_set_value(NumberWindow *numberwindow, int value);\n");
}

void number_window_set_step_size(NumberWindow *numberwindow, int step){
	printf("Start:void number_window_set_step_size(NumberWindow *numberwindow, int step);\n");
	printf("end:void number_window_set_step_size(NumberWindow *numberwindow, int step);\n");
}

int number_window_get_value(NumberWindow *numberwindow){
	printf("Start:int number_window_get_value(NumberWindow *numberwindow);\n");
	printf("end:int number_window_get_value(NumberWindow *numberwindow);\n");
}

void clock_copy_time_string(char *buffer, uint8_t size){
	printf("Start:void clock_copy_time_string(char *buffer, uint8_t size);\n");
	printf("end:void clock_copy_time_string(char *buffer, uint8_t size);\n");
}

