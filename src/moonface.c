#include <pebble.h>
#include <math.h>

typedef struct {
  int hours;
  int minutes;
} Time;

static Window *window;
static TextLayer *time_layer;
static TextLayer *date_layer;
static Layer *canvas_layer;
static Time last_time;


static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char date_buffer[] = "01/01/70";
  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  strftime(date_buffer, sizeof("01/01/70"), "%m/%d/%y", tick_time);
  // Display this time on the TextLayer
  text_layer_set_text(date_layer, date_buffer);
  text_layer_set_text(time_layer, buffer);
}

// on tick handler
static void tick_handler(struct tm *tick_time, TimeUnits changed) {
    update_time();
    if (canvas_layer) {
        layer_mark_dirty(canvas_layer);
    }
}


int julian_date(int d, int m, int y)
{
    int mm, yy;
    int k1, k2, k3;
    int j;

    yy = y - (int)((12 - m) / 10);
    mm = m + 9;
    if (mm >= 12)
    {
        mm = mm - 12;
    }
    k1 = (int)(365.25 * (yy + 4712));
    k2 = (int)(30.6001 * mm + 0.5);
    k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
    // 'j' for dates in Julian calendar:
    j = k1 + k2 + d + 59;
    if (j > 2299160)
    {
        // For Gregorian calendar:
        j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time)
    }
    return j;
}

double approximate_moon_phase(int j) {
    double ip = (j+4.867)/29.53059;
    ip = ip - floor(ip);
    return ip;
}

double moon_age(int d, int m, int y)
{
    int j = julian_date(d, m, y);
    double ag = 0;
    //Calculate the approximate phase of the moon
    double ip = approximate_moon_phase(j);
    if(ip < 0.5)
        ag = ip * 29.53059 + 29.53059 / 2;
    else
        ag = ip * 29.53059 - 29.53059 / 2;
    // Moon's age in days
    ag = floor(ag) + 1;
    return ag;
}

// haha, can't compile in libm.a i guess, lets make our own sqrt function.
long double mysqrt(long double t) {
    long double r = t/2;
    for ( int i = 0; i < 20; i++ ) {
        r = (r+t/r)/2;
    }
    return r;
}

// setup our main canvas
static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(this_layer);

    // set background
    graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, bounds.size.h), 0, GColorBlack);

    // Get the center of the screen (non full-screen)
    GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2-30));

    // make our moon circle in our canvas layer
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, center, 45);

    // we are going to make strokes line by line down the Y axis to make our dark side
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    double phase = approximate_moon_phase(julian_date(t->tm_mday,t->tm_mon+1,t->tm_year+1900));
    int y;
    for (y=-44; y<=44;y++){
        // here lets practice doing 8th grade math...
        int32_t x = floor(mysqrt(44*44-y*y));

        GPoint start_1 = GPoint(center.x-x,center.y+y);
        GPoint end_1 = GPoint(center.x+x, center.y+y);
        // color entire swath black first line
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_draw_line(ctx, start_1, end_1);


        // figure out today's moon phase
        double Rpos = 2 * x;
        double Xpos1;
        double Xpos2;
        // figure out shading.
        if (phase < 0.5)
        {
           Xpos1 = - x;
           Xpos2 = (int)(Rpos - 2*phase*Rpos - x);
        }
        else
        {
          Xpos1 = x;
          Xpos2 = (int)(x - 2*phase*Rpos + Rpos);
        }

        graphics_context_set_stroke_color(ctx, GColorWhite);

        GPoint start_2 = GPoint(center.x+Xpos1,center.y-y);
        GPoint end_2 = GPoint(center.x+Xpos2, center.y-y);
        GPoint start_3 = GPoint(center.x+Xpos1,center.y+y);
        GPoint end_3 = GPoint(center.x+Xpos2, center.y+y);

        graphics_draw_line(ctx, start_2, end_2);
        graphics_draw_line(ctx, start_3, end_3);
    }
}

// setup our main window on load
static void window_load(Window *window) {
    // get the window root layer
    Layer *window_layer = window_get_root_layer(window);
    // find the bounds of the window layer
    GRect bounds = layer_get_bounds(window_layer);

    // Create Layer
    canvas_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
    // add the canvas to the window as a child
    layer_add_child(window_layer, canvas_layer);
    // Set the update_proc
    layer_set_update_proc(canvas_layer, canvas_update_proc);
    
    // Create time TextLayer
    time_layer = text_layer_create(GRect(0, 115, 144, 110));
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_text(time_layer, "00:00");

    date_layer = text_layer_create(GRect(0, 105, 144, 100));
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_color(date_layer, GColorWhite);
    text_layer_set_text(date_layer, "01/01/1970");

    // Improve the layout to be more like a watchface
    text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);

    text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
}

static void window_unload(Window *window) {
    // remove our canvas_layer from the window on unload.
    layer_destroy(canvas_layer);
    text_layer_destroy(time_layer);
    text_layer_destroy(date_layer);
}

static void init(void) {
    // subscribe our tick handler to minutes
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    // create our "window" on the watch
    window = window_create();
    // setup our window handler
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    // setup a const animated to show we want the window animated.
    const bool animated = true;
    // push the window on the window stack
    window_stack_push(window, animated);
    update_time();
}


static void deinit(void) {
    //destroy our window
    window_destroy(window);
}

int main(void) {
    // initialize the app
    init();
    // log that we are loaded on the window
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
    // start the application event loop (standard pebble)
    app_event_loop();
    // deinitialize the application
    deinit();
}
