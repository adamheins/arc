#include <pebble.h>

/*
 * A simple watchface that displays time as two arcs around the watch,
 * one for hours and one for minutes.
 *
 * Author: Adam Heins
 */
  
static Window *s_main_window;
static InverterLayer *s_invert_layer;
static Layer *s_draw_layer;


/*
 * Draws a pixel, as long as it falls between 90 degrees and the given angle.
 */
static void draw_pixel(GContext *ctx, GPoint point, GPoint center, 
        int32_t angle) {

    int32_t point_angle = atan2_lookup(point.y - center.y, point.x - center.x);
    if (point_angle + TRIG_MAX_ANGLE / 4 < angle 
        || (point_angle - TRIG_MAX_ANGLE * 0.75 < angle 
            && point_angle - TRIG_MAX_ANGLE * 0.75 > 0))
    graphics_draw_pixel(ctx, point);
}


/*
 * Draws an arc between 90 degrees and an angle.
 */
static void draw_arc(GContext *ctx, GPoint center, int32_t r, 
        int32_t to_angle) {

    int32_t x = r;
    int32_t y = 0;
    int32_t r_error = 1 - x;

    while (x >= y) {
        draw_pixel(ctx, GPoint(x + center.x, y + center.y), center, to_angle);
        draw_pixel(ctx, GPoint(y + center.x, x + center.y), center, to_angle);
        draw_pixel(ctx, GPoint(-x + center.x, y + center.y), center, to_angle);
        draw_pixel(ctx, GPoint(-y + center.x, x + center.y), center, to_angle);
        draw_pixel(ctx, GPoint(-x + center.x, -y + center.y), center, to_angle);
        draw_pixel(ctx, GPoint(-y + center.x, -x + center.y), center, to_angle);
        draw_pixel(ctx, GPoint(x + center.x, -y + center.y), center, to_angle);
        draw_pixel(ctx, GPoint(y + center.x, -x + center.y), center, to_angle);
        ++y;
        if (r_error < 0) {
            r_error += (y + 1) << 1;
        } else {
            --x;
            r_error += (y - x + 1) << 1;
        }
    }
}


/*
 * Draws an arc that can be multiple pixels wide.
 * Contains some artifacts (adds to the charm).
 */
static void draw_thick_arc(GContext *ctx, GPoint center, int32_t inner_radius, 
        int32_t outer_radius, int32_t to_angle) {
    for (int32_t i = inner_radius; i < outer_radius; ++i)
        draw_arc(ctx, center, i, to_angle);
}


/*
 * Update proc for the layer on which the arc is drawn, s_draw_layer.
 */
static void draw_update_proc(Layer *this_layer, GContext *ctx) {
  
    // Distances to each circle.
    int32_t hours_dist = 44;
    int32_t minutes_dist = 54;

    // Get the time.
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    int32_t hours = tick_time->tm_hour;
    int32_t minutes = tick_time->tm_min;

    GRect bounds = layer_get_bounds(this_layer);
    GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

    // Draw the delightful looking arcs.
    graphics_context_set_stroke_color(ctx, GColorWhite);
    draw_thick_arc(ctx, center, 30, 36, TRIG_MAX_ANGLE * (hours % 12) / 12);
    draw_thick_arc(ctx, center, 52, 58, TRIG_MAX_ANGLE * minutes / 60);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_change) {
    layer_mark_dirty(s_draw_layer);
}


static void main_window_load(Window *window) {

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
  
    s_invert_layer = inverter_layer_create(bounds);

    s_draw_layer = layer_create(bounds);
    layer_set_update_proc(s_draw_layer, draw_update_proc);

    layer_add_child(window_layer, inverter_layer_get_layer(s_invert_layer));
    layer_add_child(window_layer, s_draw_layer);
}


static void main_window_unload(Window *window) {
    layer_destroy(s_draw_layer);
    inverter_layer_destroy(s_invert_layer);
}
  

static void init() {
    s_main_window = window_create();

    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });

    window_stack_push(s_main_window, true);

    // Register with TickTimerService.
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}


static void deinit() {
    window_destroy(s_main_window);
}


int main(void) {
    init();
    app_event_loop();
    deinit();
}