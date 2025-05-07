/*
* Copyright (C) 2011 Benjamin Franzke
* Copyright 2018,2022 NXP
*
* Permission to use, copy, modify, distribute, and sell this software and its
* documentation for any purpose is hereby granted without fee, provided that
* the above copyright notice appear in all copies and that both that copyright
* notice and this permission notice appear in supporting documentation, and
* that the name of the copyright holders not be used in advertising or
* publicity pertaining to distribution of the software without specific,
* written prior permission. The copyright holders make no representations
* about the suitability of this software for any purpose. It is provided "as
* is" without express or implied warranty.
*
* THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
* INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
* EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
* CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
* DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
* TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
* OF THIS SOFTWARE.
*/

#include "display.hpp"

#include <stdbool.h>
#include <math.h>
#include <signal.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#include "xdg-shell-client-protocol.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

struct window_geometry {
	int width; 
	int height;
};

struct wayland_display;

struct display_window {
	struct wayland_display *display;
	struct wl_egl_window *native;
	struct window_geometry geometry, window_size;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	EGLSurface egl_surface;
	struct wl_callback *callback;
	int fullscreen, configured;
};

struct wayland_display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct xdg_wm_base *wm_base;
	struct wl_seat *seat;
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
	struct wl_shm *shm;
	struct wl_cursor_theme *cursor_theme;
	struct wl_cursor *default_cursor;
	struct wl_surface *cursor_surface;
	struct display_window *window;
};

static struct wayland_display sdisplay = { 0 };
static struct display_window swindow = { 0 };

static void
handle_xdg_surface_configure(void *data, struct xdg_surface *surface,
			     uint32_t serial)
{
	wayland_display *window = (wayland_display *)data;

	xdg_surface_ack_configure(surface, serial);

}

static const struct xdg_surface_listener xdg_surface_listener = {
	handle_xdg_surface_configure,
};

static void
handle_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
			      int32_t width, int32_t height,
			      struct wl_array *state)
{
}

static void
handle_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	handle_xdg_toplevel_configure,
	handle_xdg_toplevel_close,
};

static void
create_xdg_surface(void)
{
	struct wayland_display * display = &sdisplay;
	struct display_window * window = &swindow;

	window->surface = wl_compositor_create_surface(display->compositor);

	window->xdg_surface =
		xdg_wm_base_get_xdg_surface(display->wm_base,
						window->surface);
	assert(window->xdg_surface);
	xdg_surface_add_listener(window->xdg_surface,
					&xdg_surface_listener, window);

	window->xdg_toplevel =
		xdg_surface_get_toplevel(window->xdg_surface);
	assert(window->xdg_toplevel);
	xdg_toplevel_add_listener(window->xdg_toplevel,
					&xdg_toplevel_listener, window);

	xdg_toplevel_set_title(window->xdg_toplevel, "Surround View 1.2");
	xdg_toplevel_set_app_id(window->xdg_toplevel,
			"SV_App");

	xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);

	wl_surface_commit(window->surface);

	window->native =
		wl_egl_window_create(window->surface,
						window->window_size.width,
						window->window_size.height);

}

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface,
		     wl_fixed_t sx, wl_fixed_t sy)
{
	struct wayland_display *display = (struct wayland_display *)data;
	struct wl_buffer *buffer;
	struct wl_cursor *cursor = display->default_cursor;
	struct wl_cursor_image *image;

	if (display->window->fullscreen != (int)0) {
		wl_pointer_set_cursor(pointer, serial, NULL, 0, 0);
	}
	else {
		if (cursor != NULL) {
			image = display->default_cursor->images[0];
			buffer = wl_cursor_image_get_buffer(image);
			wl_pointer_set_cursor(pointer, serial,
						display->cursor_surface,
						(int32_t)image->hotspot_x,
						(int32_t)image->hotspot_y);
			wl_surface_attach(display->cursor_surface, buffer, 0, 0);
			wl_surface_damage(display->cursor_surface, 0, 0,
						(int32_t)image->width, (int32_t)image->height);
			wl_surface_commit(display->cursor_surface);
		}
	}
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
		     uint32_t serial, struct wl_surface *surface)
{
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
		      uint32_t handle_time, wl_fixed_t sx, wl_fixed_t sy)
{
}

static void
pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
		      uint32_t serial, uint32_t handle_time, uint32_t button,
		      uint32_t state)
{
	struct wayland_display *display = (struct wayland_display *)data;

	if ((button == (uint32_t)BTN_LEFT) && (state == (uint32_t)WL_POINTER_BUTTON_STATE_PRESSED)) {
		xdg_toplevel_move(display->window->xdg_toplevel,display->seat,serial);
	}
}

static void
pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
		    uint32_t handle_time, uint32_t handle_axis, wl_fixed_t value)
{
}

static const struct wl_pointer_listener pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis,
};

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
		       uint32_t format, int fd, uint32_t size)
{
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
		      uint32_t serial, struct wl_surface *surface,
		      struct wl_array *keys)
{
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
		      uint32_t serial, struct wl_surface *surface)
{
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
		    uint32_t serial, uint32_t handle_time, uint32_t key,
		    uint32_t state)
{
	struct wayland_display *d = (struct wayland_display *)data;
/* Remove fullscreen toggle, need to port toggle to xdg backend
	if ((key == (uint32_t)KEY_F11) && (state != 0U)) {
		toggle_fullscreen(d->window, (int)((uint)(d->window->fullscreen) ^ 1U));
	}
*/
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
			  uint32_t serial, uint32_t mods_depressed,
			  uint32_t mods_latched, uint32_t mods_locked,
			  uint32_t group)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
};


static void 
seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps)
{
	struct wayland_display *d = (struct wayland_display *)data;

	if (((caps & (uint32_t)WL_SEAT_CAPABILITY_POINTER) != 0U) && (d->pointer == NULL)) {
		d->pointer = wl_seat_get_pointer(seat);
		assert(wl_pointer_add_listener(d->pointer, &pointer_listener, d) >=0 );
	} else {
		if (((caps & (uint32_t)WL_SEAT_CAPABILITY_POINTER) == 0U) && (d->pointer != NULL)) {
			wl_pointer_destroy(d->pointer);
			d->pointer = NULL;
		}
	}

	if (((caps & (uint32_t)WL_SEAT_CAPABILITY_KEYBOARD) != 0U) && (d->keyboard == NULL)) {
		d->keyboard = wl_seat_get_keyboard(seat);
		if(wl_keyboard_add_listener(d->keyboard, &keyboard_listener, d) == -1) {
			cout << "wl_keyboard_add_listener error" << endl;
		}
	} else {
		if (((caps & (uint32_t)WL_SEAT_CAPABILITY_KEYBOARD) == 0U) && (d->keyboard != NULL)) {
			wl_keyboard_destroy(d->keyboard);
			d->keyboard = NULL;
		}
	}
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
	xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	xdg_wm_base_ping,
};

static void
registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	struct wayland_display *d = (struct wayland_display *)data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor = (struct wl_compositor*)wl_registry_bind(registry, name,&wl_compositor_interface, 1);
	}
	else {
		if (strcmp(interface, "xdg_wm_base") == 0) {
			
    		d->wm_base = (struct xdg_wm_base *)wl_registry_bind(registry, name, &xdg_wm_base_interface, min(version, 1));
    		xdg_wm_base_add_listener(d->wm_base, &xdg_wm_base_listener, d);

		}
		else {
			if (strcmp(interface, "wl_seat") == 0) {
				d->seat = (struct wl_seat *)wl_registry_bind(registry, name, &wl_seat_interface, 1);
				if(wl_seat_add_listener(d->seat, &seat_listener, d) == -1) {
					cout << "Listener was not set" << endl;
				}
			}
			else {
				if (strcmp(interface, "wl_shm") == 0) {
					d->shm = (struct wl_shm *)wl_registry_bind(registry, name, &wl_shm_interface, 1);
					d->cursor_theme = wl_cursor_theme_load(NULL, 32, d->shm);
					d->default_cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "left_ptr");
				}
			}
		}
	}
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global
};


/**************************************************************************************************************
 *
 * @brief  			MyDisplay class constructor.
 *
 * @param  in 		const int width - window width
 *					const int height - window height
 *					const char* keyboard_dev - keyboard device file from /dev/input/by-path folder
 *					const char* mouse_dev - mouse device file from /dev/input/by-path folder
 *					const int msaa - MSAA samples count
 *
 * @return 			The function creates the MyDisplay object.
 *
 * @remarks 		The function creates eglNativeDisplayType display. 
 *					If X11 window system is used then Display* display is created. 
 *					In this case keyboard and mouse devices ignored because X11 server is used for event catching. 
 *					If FB backend is used then /dev/fb0 is used for rendering.
 *					In this case it is necessary to set correct keyboard and mouse devices for event catching.
 *
 **************************************************************************************************************/
MyDisplay::MyDisplay(const int width, const int height, const char* keyboard_dev, const char* mouse_dev, const int msaa)
{
	// Keyboard device
	fd_k = open(keyboard_dev, O_RDONLY | O_NONBLOCK);
	if (fd_k == -1) 
	{
		cout << "Failed to open input device" << endl;
	}
	
	// Mouse device
	fd_m = open(mouse_dev, O_RDONLY | O_NONBLOCK);
	if (fd_m == -1) 
	{
		cout << "Failed to open input device" << endl;
	}
	
	btn_mouse_left = false;

	mouse_offset[0] = 0.0f;
	mouse_offset[1] = 0.0f;

	dispInit(width, height, msaa);
}

/**************************************************************************************************************
 *
 * @brief  			MyDisplay class destructor.
 *
 * @param			-
 *
 * @return 			-
 *
 * @remarks 		The function sets free all egl objects and closes display
 *
 **************************************************************************************************************/
MyDisplay::~MyDisplay(void)
{
	assert(eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_TRUE);
	assert(eglTerminate(egldisplay) == EGL_TRUE);
	assert(eglReleaseThread() == EGL_TRUE);
	
	struct wayland_display * display = &sdisplay;
	struct display_window * window = &swindow;
		wl_egl_window_destroy(window->native);
		xdg_surface_destroy(window->xdg_surface);
		wl_surface_destroy(window->surface);
		if (window->callback != NULL) {
			wl_callback_destroy(window->callback);
		}
	
	wl_surface_destroy(display->cursor_surface);
	
	if (display->cursor_theme != NULL) {
		wl_cursor_theme_destroy(display->cursor_theme);
	}

	if (display->wm_base != NULL) {
		xdg_toplevel_destroy(window->xdg_toplevel);
	}

	if (display->compositor != NULL) {
		wl_compositor_destroy(display->compositor);
	}

	assert(wl_display_flush(display->display) >= 0);
	wl_display_disconnect(display->display);

	if(close(fd_k) != 0) { cout << "Keyboard device was not closed" << endl; }
	if(close(fd_m) != 0) { cout << "Mouse device was not closed" << endl; }
}

/**************************************************************************************************************
 *
 * @brief  			post EGL surface color buffer to a native window
 *
 * @param   		-
 *
 * @return 			-
 *
 * @remarks 		the function posts surface color buffer to the associated native window. It performs an
 *					implicit glFlush before returning.
 *
 **************************************************************************************************************/
void MyDisplay::swapBuffers(void)
{
	//struct wayland_display* display = &sdisplay;
	//wl_display_dispatch(display->display);

	if(!eglSwapBuffers(egldisplay, eglsurface)) {
		cout << "eglSwapBuffers failed " << endl;
	}
}

/**************************************************************************************************************
 *
 * @brief  			Native display and window initalization
 *
 * @param  in 		const int width - window width
 *					const int height - window height
 *					const int msaa - MSAA samples count
 *
 * @return 			-
 *
 * @remarks 		The function creates eglNativeDisplayType display. 
 *					If X11 window system is used then Display* display is created. 
 *					In this case keyboard and mouse devices ignored because X11 server is used for event catching. 
 *					If FB backend is used then /dev/fb0 is used for rendering.
 *
 **************************************************************************************************************/
void MyDisplay::dispInit(const int width, const int height, const int msaa)
{
	static const EGLint s_configAttribs[] =
	{
		EGL_SAMPLES, msaa,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 0,
		EGL_DEPTH_SIZE, 8,
		EGL_SURFACE_TYPE, 
		EGL_WINDOW_BIT,
		EGL_NONE
	};

	EGLint numconfigs;
	EGLNativeWindowType native_window = (EGLNativeWindowType)0;
		
	sdisplay.display = wl_display_connect(NULL);
	if (sdisplay.display  == NULL)
    {
        cout << ("wl_display_connect failed") << endl;
        exit(EXIT_FAILURE);
    }
	sdisplay.registry = wl_display_get_registry(sdisplay.display);
	assert(wl_registry_add_listener(sdisplay.registry, &registry_listener, &sdisplay) >= 0);
	assert(wl_display_dispatch(sdisplay.display) >= 0);
	eglNativeDisplayType = (EGLNativeDisplayType)(sdisplay.display);

	swindow.window_size.width  = width;
	swindow.window_size.height = height;
	swindow.fullscreen = 1;

	swindow.display = &sdisplay;
	sdisplay.window = &swindow;

	sdisplay.registry = wl_display_get_registry(sdisplay.display);
	assert(wl_registry_add_listener(sdisplay.registry, &registry_listener, &sdisplay) >= 0);
	assert(wl_display_dispatch(sdisplay.display) >= 0);
	create_xdg_surface();
	native_window = (EGLNativeWindowType )(swindow.native);
	sdisplay.cursor_surface = wl_compositor_create_surface(sdisplay.compositor);

	egldisplay = eglGetDisplay(eglNativeDisplayType);

	assert(eglInitialize(egldisplay, NULL, NULL) == EGL_TRUE);
	assert(eglBindAPI(EGL_OPENGL_ES_API) == EGL_TRUE);
	assert(eglChooseConfig(egldisplay, s_configAttribs, &eglconfig, 1, &numconfigs) == EGL_TRUE);
	assert(numconfigs == 1);

	eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, native_window, NULL);

	assert(eglGetError() == EGL_SUCCESS);
	EGLint ContextAttribList[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	eglcontext = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, ContextAttribList);
	assert(eglGetError() == EGL_SUCCESS);

	assert(eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext) == EGL_TRUE);
}

/**************************************************************************************************************
 *
 * @brief  			Get number of unprocessed events for x11 or flag of an input event for fb backend
 *
 * @param   		-
 *
 * @return 			int		The number of unprocessed events for x11 backend
 *							The flag of an input event for fb backend 
 *							(1 - there is at least one unprocessed event, 0 - there isn't any unprocessed event)
 *
 * @remarks 		The function returns the number of unprocessed events for x11 backend or the flag of an 
 *					input event for fb backend. The function checks events from keyboard and mouse devices.
 *					If there isn't any unprocessed event, the function returns 0.
 *
 **************************************************************************************************************/
int MyDisplay::getEventsNum(void)
{
	// Check keyboard events
	if (read(fd_k, &inevent, sizeof(inevent)) == (ssize_t)sizeof(inevent)) {
		if ((inevent.type == (uint)EV_KEY) && (inevent.value == 1)) {
			return(1); 
		}
	}

	// Check mouse events
	if (read(fd_m, &inevent, sizeof(inevent)) == (ssize_t)sizeof(inevent))
	{
		if ((inevent.type == (uint)EV_REL) || (inevent.type == (uint)EV_KEY)) {
				return (1);
		}
	}
	
	return(0);
}

/**************************************************************************************************************
 *
 * @brief  			Get code of current captured event
 *
 * @param   		-
 *
 * @return 			ev_type		code of current captured event: m_move - mouse moving + mouse left button pressing
 *																m_scroll_up - mous up scrolling
 *																m_scroll_down - mouse down scrolling
 *																k_esc - Esc key pressing
 *																k_up - Up key pressing
 *																k_down - Down key pressing
 *																k_right - Right key pressing
 *																k_left - Left key pressing
 *																k_f1 - F1 key pressing
 *																ev_none - other non classificate events
 *
 * @remarks 		The function returns checks current event description and returns event type.
 *
 **************************************************************************************************************/
ev_type MyDisplay::getNextEvent(void)
{
	
	ev_type ret_val = ev_none;
	
	if (inevent.type == (uint)EV_KEY) {// Key pressing event
		switch(inevent.code)
		{
			case KEY_ESC:
				ret_val = k_esc;
				break;
			case KEY_LEFT:
				ret_val = k_left;
				break;
			case KEY_RIGHT:
				ret_val = k_right;
				break;
			case KEY_UP:
				ret_val = k_up;
				break;
			case KEY_DOWN:
				ret_val = k_down;
				break;
			case KEY_F1:
				ret_val = k_f1;
				break;
			case KEY_F5: 
				ret_val = k_f5;
				break;
			case KEY_P: 
				ret_val = k_p;
				break;
			case BTN_LEFT: // Mouse left button pressing
				if (inevent.value == 1)
					{ btn_mouse_left = true; }
				else { if (inevent.value == 0) { btn_mouse_left = false; } }
				break;
			default:
				ret_val = ev_none;
				break;
		}
	}
	else {
		if (inevent.type == (uint)EV_REL) {// Mouse event
			switch (inevent.code)
			{
				case REL_WHEEL: // Mouse scrolling
					if(inevent.value == 1)
						{ ret_val = m_scroll_up; }
					else { if (inevent.value == -1) { ret_val = m_scroll_down; } }
					break;
				case REL_X: // Mouse movement wit left button pressed
					if (btn_mouse_left)
					{
						if((inevent.value < 10) || (inevent.value > -10))
							{ mouse_offset[0] = 2.0f * (float)inevent.value; }
						else
							{ mouse_offset[0] = 0.0f; }
						mouse_offset[1] = 0.0f;
						ret_val = m_move;
					}
					else { ret_val = ev_none; }
					break;
				case REL_Y: // Mouse movement wit left button pressed
					if (btn_mouse_left)
					{
						if ((inevent.value < 10) || (inevent.value > -10))
							{ mouse_offset[1] = -2.0f * (float)inevent.value; }
						else
							{ mouse_offset[1] = 0.0f; }
						mouse_offset[0] = 0.0f;
						ret_val = m_move;
					}
					else { ret_val = ev_none; }
					break;
				default:
					ret_val = ev_none;
					break;
			}
		}
	}
	return (ret_val);
}

EGLDisplay MyDisplay::getEGLDispaly(){
	return this->egldisplay;
}

EGLContext MyDisplay::getEGLContext(){
	return this->eglcontext;
}

