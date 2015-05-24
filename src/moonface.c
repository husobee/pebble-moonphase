#include <pebble.h>

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
