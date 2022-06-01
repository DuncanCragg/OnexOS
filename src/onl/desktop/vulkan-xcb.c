
/* Wiring between Vulkan and XCB */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <xcb/xcb.h>

#include "onl/onl.h"

xcb_connection_t *connection;
xcb_screen_t *    screen;
xcb_window_t      window;

xcb_intern_atom_reply_t *atom_wm_delete_window;

void onl_init()
{
  const xcb_setup_t *setup;
  xcb_screen_iterator_t iter;
  int scr;

  connection = xcb_connect(NULL, &scr);
  if(xcb_connection_has_error(connection)){
      printf("Cannot connect to XCB\n");
      fflush(stdout);
      exit(1);
  }

  setup = xcb_get_setup(connection);
  iter = xcb_setup_roots_iterator(setup);
  while(scr--) xcb_screen_next(&iter);
  screen = iter.data;
}

#define WIDTH 1920
#define HEIGHT 1024
void onl_create_window()
{
  uint32_t value_mask, value_list[32];

  window = xcb_generate_id(connection);

  value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  value_list[0] = screen->black_pixel;
  value_list[1] =
    XCB_EVENT_MASK_KEY_RELEASE |
    XCB_EVENT_MASK_KEY_PRESS |
    XCB_EVENT_MASK_EXPOSURE |
    XCB_EVENT_MASK_STRUCTURE_NOTIFY |
    XCB_EVENT_MASK_POINTER_MOTION |
    XCB_EVENT_MASK_BUTTON_PRESS |
    XCB_EVENT_MASK_BUTTON_RELEASE;

  xcb_create_window(connection,
    XCB_COPY_FROM_PARENT,
    window, screen->root,
    0, 0, WIDTH, HEIGHT, 0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT,
    screen->root_visual,
    value_mask, value_list);

  /* Magic code that will send notification when window is destroyed */
  xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, 0);
  xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
  atom_wm_delete_window = xcb_intern_atom_reply(connection, cookie2, 0);

  xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
    window, (*reply).atom, 4, 32, 1,
    &(*atom_wm_delete_window).atom);

  free(reply);

  xcb_map_window(connection, window);
}

void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface) {
    VkResult err;

    VkXcbSurfaceCreateInfoKHR xcb_surface_ci = {
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .flags = 0,
      .connection = connection,
      .window = window,
    };

    err = vkCreateXcbSurfaceKHR(inst, &xcb_surface_ci, 0, surface);
    assert(!err);
}

extern void ont_vk_handle_event(char key, int w, int h);

bool quit = false;
bool pause =false;

static void handle_xcb_event(const xcb_generic_event_t *event) {
    uint8_t event_code = event->response_type & 0x7f;
    switch (event_code) {
        case XCB_EXPOSE:
            // TODO: Resize window
            break;
        case XCB_CLIENT_MESSAGE:
            if ((*(xcb_client_message_event_t *)event).data.data32[0] == (*atom_wm_delete_window).atom) {
                quit = true;
            }
            break;
        case XCB_KEY_RELEASE: {
            const xcb_key_release_event_t *key = (const xcb_key_release_event_t *)event;
            switch (key->detail) {
                case 0x9:  // Escape
                    quit = true;
                    break;
                case 0x41:  // space bar
                    pause = !pause;
                    break;
                default:
                    ont_vk_handle_event(key->detail, 0,0);
                    break;
            }
        } break;
        case XCB_CONFIGURE_NOTIFY: {
            const xcb_configure_notify_event_t *cfg = (const xcb_configure_notify_event_t *)event;
            ont_vk_handle_event(0, cfg->width, cfg->height);
        } break;
        default:
            break;
    }
}

extern void ont_vk_loop();

void onl_run() {

    xcb_flush(connection);

    while (!quit) {

        xcb_generic_event_t *event;

        if (pause) {
            event = xcb_wait_for_event(connection);
        } else {
            event = xcb_poll_for_event(connection);
        }
        while (event) {
            handle_xcb_event(event);
            free(event);
            event = xcb_poll_for_event(connection);
        }
        ont_vk_loop();
    }
}

void onl_finish()
{
  xcb_destroy_window(connection, window);
  xcb_disconnect(connection);
  free(atom_wm_delete_window);
}
