
/* Wiring between Vulkan and XCB */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <xcb/xcb.h>

#include "ont/vulkan/vulkan_up.h"
#include "onl/onl.h"

iostate io;

xcb_connection_t *connection;
xcb_screen_t *    screen;
xcb_window_t      window;

xcb_intern_atom_reply_t *atom_wm_delete_window;

bool quit = false;

static void sighandler(int signal, siginfo_t *siginfo, void *userdata) {
  printf("\nEnd\n");
  quit = true;
}

void onl_init() {

  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_sigaction = sighandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &act, 0);

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

void onl_create_window()
{
  io.swap_width =1920/2;
  io.swap_height= 1080/2;

  set_io_rotation(0);

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
    0, 0, io.swap_width, io.swap_height, 0,
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

static void handle_xcb_event(const xcb_generic_event_t *event) {

    switch(event->response_type & 0x7f) {

        case XCB_MOTION_NOTIFY: {
          xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
          set_io_mouse((int32_t)motion->event_x, (int32_t)motion->event_y);
          ont_vk_iostate_changed();
          break;
        }
        case XCB_BUTTON_PRESS: {
          xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
          if(press->detail == XCB_BUTTON_INDEX_1) io.left_pressed=true;
          if(press->detail == XCB_BUTTON_INDEX_2) io.middle_pressed=true;
          if(press->detail == XCB_BUTTON_INDEX_3) io.right_pressed=true;
          ont_vk_iostate_changed();
          break;
        }
        case XCB_BUTTON_RELEASE: {
          xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
          if(press->detail == XCB_BUTTON_INDEX_1) io.left_pressed=false;
          if(press->detail == XCB_BUTTON_INDEX_2) io.middle_pressed=false;
          if(press->detail == XCB_BUTTON_INDEX_3) io.right_pressed=false;
          ont_vk_iostate_changed();
          break;
        }
        case XCB_KEY_PRESS: {
            const xcb_key_press_event_t *key = (const xcb_key_press_event_t *)event;
            switch (key->detail) {
                case 0x9:  // ESC
                    break;
                default:
                    io.key=key->detail;
                    ont_vk_iostate_changed();
                    break;
            }
            break;
        }
        case XCB_KEY_RELEASE: {
            const xcb_key_release_event_t *key = (const xcb_key_release_event_t *)event;
            switch (key->detail) {
                case 0x9:  // ESC
                    quit = true;
                    break;
                default:
                    io.key=0;
                    ont_vk_iostate_changed();
                    break;
            }
            break;
        }
        case XCB_CONFIGURE_NOTIFY: {
            const xcb_configure_notify_event_t *cfg = (const xcb_configure_notify_event_t *)event;
            uint32_t w=cfg->width;
            uint32_t h=cfg->height;
            if ((io.view_width != w) || (io.view_height != h)) {

                io.swap_width = w;
                io.swap_height = h;

                set_io_rotation(io.rotation_angle);

                ont_vk_iostate_changed();
            }
            break;
        }
        case XCB_CLIENT_MESSAGE:{
            if ((*(xcb_client_message_event_t *)event).data.data32[0] == (*atom_wm_delete_window).atom) {
                quit = true;
            }
            break;
        }
        case XCB_EXPOSE: {
            break;
        }
        default: {
            break;
        }
    }
}

void onl_run() {

    xcb_flush(connection);

    while (!quit) {

        xcb_generic_event_t *event;

        while((event = xcb_poll_for_event(connection))) {

            handle_xcb_event(event);
            free(event);
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
