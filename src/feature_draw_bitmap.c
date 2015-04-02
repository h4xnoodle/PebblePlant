/*
 * The original source image is from:
 *   <http://openclipart.org/detail/26728/aiga-litter-disposal-by-anonymous>
 *
 * The source image was converted from an SVG into a RGB bitmap using
 * Inkscape. It has no transparency and uses only black and white as
 * colors.
 */

#include "pebble.h"

#define KEY_PLANT_STATE 1337
#define WAKEUP_ID 0
  
static Window *s_main_window;
static Layer *s_image_layer;
static GBitmap *s_image;

typedef enum {
  ALIVE = 0,
  THIRSTY = 1,
  PARCHED = 2,
  DYING = 3,
  DEAD = 9
} PlantState_t;

static PlantState_t current_state = ALIVE;
static int32_t wakeup_id;

uint32_t time_limit_for_state(PlantState_t state);
static void change_image(uint32_t image);
static uint32_t image_for_state(PlantState_t state);
static void change_state();

static void layer_update_callback(Layer *layer, GContext* ctx) {
  // We make sure the dimensions of the GRect to draw into
  // are equal to the size of the bitmap--otherwise the image
  // will automatically tile. Which might be what *you* want.
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Layer update callback");

#ifdef PBL_PLATFORM_BASALT
  GSize image_size = gbitmap_get_bounds(s_image).size;
#else 
  GSize image_size = s_image->bounds.size;
#endif

  GRect frame = layer_get_frame(s_image_layer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Size of image: %d %d", image_size.w, image_size.h);
  graphics_draw_bitmap_in_rect(ctx, s_image, GRect(10, 10, image_size.w, image_size.h));
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_frame(window_layer);

  s_image_layer = layer_create(bounds);
  layer_set_update_proc(s_image_layer, layer_update_callback);
  layer_add_child(window_layer, s_image_layer);

  change_image(image_for_state(current_state));
}

static void main_window_unload(Window *window) {
  gbitmap_destroy(s_image);
  layer_destroy(s_image_layer);
}

static void change_image(uint32_t new_image) {
  gbitmap_destroy(s_image);
  s_image = gbitmap_create_with_resource(new_image);
  layer_mark_dirty(s_image_layer);
}

static void wakeup_handler(WakeupId id, int32_t reasonCode) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Wakeup happened");
  change_state();
  change_image(image_for_state(current_state));
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  wakeup_service_subscribe(wakeup_handler);
  
  time_t future = time(NULL) + time_limit_for_state(current_state);
  wakeup_id = wakeup_schedule(future, WAKEUP_ID, true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Wake up scheduled");
}

static void change_state() {
  switch (current_state) {
    case ALIVE:
      current_state = THIRSTY;
      break;
    case THIRSTY:
      current_state = PARCHED;
      break;
    case PARCHED:
      current_state = DYING;
    case DYING:
    case DEAD:
    default:
      current_state = DEAD;
  }
  persist_write_int(KEY_PLANT_STATE, (int32_t) current_state);
}

uint32_t time_limit_for_state(PlantState_t state) {
  return 3;
}

static uint32_t image_for_state(PlantState_t state) {
  switch (state) {
    case ALIVE:
    case THIRSTY:
    case PARCHED:
      return RESOURCE_ID_IMAGE_LEAF;
    case DYING:
    case DEAD:
    default:
      return RESOURCE_ID_IMAGE_CAKE;
  }
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
