#include "stubbed-surface.h"
#include "libwayland-shim.h"
#include "xdg-surface-server.h"

#include <wayland-client.h>
#include <xdg-shell-client.h>
#include <stdlib.h>
#include <stdio.h>

struct stubbed_surface_t {
    struct xdg_surface_server_t super;
};

static void stubbed_configure_callback(void *data, struct wl_callback *callback, uint32_t callback_data) {
    (void)callback, (void)callback_data;
    struct stubbed_surface_t* self = data;

    xdg_surface_server_send_configure(&self->super, 0, 0, 100, 100, 0);
}

static const struct wl_callback_listener stubbed_configure_callback_listener = { stubbed_configure_callback };

static void stubbed_role_created(struct xdg_surface_server_t* super) {
    struct stubbed_surface_t* self = (void*)super;

    if (self->super.xdg_popup) {
        fprintf(stderr, "stubbed popup not displayed\n");
    } else if (self->super.xdg_toplevel) {
        fprintf(stderr, "stubbed toplevel not displayed\n");
    }

    // We need to send the initial configure event. If we send it now, GTK will get it before it gets returned the
    // popup the configure is on, which obviously wont work. By creating a wl_callback we can bounce a message
    // back from the compositor and send configure when it arrives, which solves this problem. HOWEVER the GDK
    // surface is dispatching on a specific event queue. Our callback needs to be on the same queue, or else it
    // wont get processed and everything will lock up.
    struct wl_event_queue* queue = wl_proxy_get_queue((struct wl_proxy*)self->super.xdg_surface);
    struct wl_display* display = libwayland_shim_proxy_get_display((struct wl_proxy*)self->super.xdg_surface);
    struct wl_display* wrapped_display = wl_proxy_create_wrapper(display);
    wl_proxy_set_queue((struct wl_proxy*)wrapped_display, queue);
    struct wl_callback* callback = wl_display_sync(wrapped_display);
    wl_callback_add_listener(callback, &stubbed_configure_callback_listener, self);
    wl_proxy_wrapper_destroy(wrapped_display);
}

void stubbed_surface_destroyed(struct xdg_surface_server_t* super) {
    struct stubbed_surface_t* self = (void*)super;
    free(self);
}

struct wl_proxy* stubbed_surface_init(struct xdg_wm_base* creating_object, struct wl_surface* surface) {
    struct stubbed_surface_t* self = calloc(1, sizeof(struct stubbed_surface_t));
    self->super.popup_created = stubbed_role_created;
    self->super.toplevel_created = stubbed_role_created;
    return xdg_surface_server_get_xdg_surface(&self->super, creating_object, surface);
}
