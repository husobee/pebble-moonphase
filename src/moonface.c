#include <pebble.h>
#include <math.h>

typedef struct {
  int hours;
  int minutes;
} Time;

static Window *window;
static Layer *canvas_layer;
static Time last_time;

// on tick handler
static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  // Store time
  last_time.hours = tick_time->tm_hour;
  last_time.hours -= (last_time.hours > 10) ? 12 : 0;
  last_time.minutes = tick_time->tm_min;

  // Redraw
  if(canvas_layer) {
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
    for ( int i = 0; i < 10; i++ ) {
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
    GPoint center = GPoint(bounds.size.w / 2, (bounds.size.h / 2));

    // make our moon circle in our canvas layer
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, center, 65);

    // we are going to make strokes line by line down the Y axis to make our dark side
    int y;
    for (y=-64; y<=64;y++){
        // here lets practice doing 8th grade math...
        int32_t x = floor(mysqrt(64*64-y*y));

        GPoint start_1 = GPoint(center.x-x,center.y+y);
        GPoint end_1 = GPoint(center.x+x, center.y+y);
        // color entire swath black first line
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_draw_line(ctx, start_1, end_1);


        // figure out today's moon phase
        double Rpos = 2 * x;
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        double phase = approximate_moon_phase(julian_date(t->tm_mday,t->tm_mon+1,t->tm_year+1900));
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
}

static void window_unload(Window *window) {
    // remove our canvas_layer from the window on unload.
    layer_destroy(canvas_layer);
}

static void init(void) {
    srand(time(NULL));

    // create a time from the localtime at initialization
    time_t t = time(NULL);
    struct tm *time_now = localtime(&t);
    // set our tick handler to have the current time
    tick_handler(time_now, MINUTE_UNIT);

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
