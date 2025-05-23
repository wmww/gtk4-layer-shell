#include "stubbed-surface.h"
#include "libwayland-shim.h"
#include "xdg-surface-server.h"
#include "layer-surface.h"
#include "registry.h"

#include <wayland-client.h>
#include <xdg-shell-client.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct geom_point_t {
    int x, y;
};

struct geom_rect_t {
    struct geom_point_t top_left;
    struct geom_size_t size;
};

struct stubbed_surface_t {
    struct xdg_surface_server_t super;
    struct geom_rect_t configure;
    struct wl_subsurface* subsurface;
};

static struct {
    void* wayland_object;
    struct geom_size_t size;
    struct geom_rect_t anchor_rect;
    enum xdg_positioner_anchor anchor;
    enum xdg_positioner_gravity gravity;
    uint32_t constraint_adjustment;
    struct geom_point_t offset;
} current_positioner;

static void stubbed_configure_callback(void *data, struct wl_callback *callback, uint32_t callback_data) {
    (void)callback, (void)callback_data;
    struct stubbed_surface_t* self = data;

    xdg_surface_server_send_configure(
        &self->super,
        self->configure.top_left.x, self->configure.top_left.y,
        self->configure.size.width, self->configure.size.height,
        0
    );
}

static const struct wl_callback_listener stubbed_configure_callback_listener = { stubbed_configure_callback };

static void stubbed_role_created(struct xdg_surface_server_t* super) {
    struct stubbed_surface_t* self = (void*)super;

    if (self->super.xdg_popup) {
        self->configure.top_left = current_positioner.anchor_rect.top_left;
        self->configure.size = current_positioner.size;
    } else if (self->super.xdg_toplevel) {
        fprintf(stderr, "stubbed toplevel not displayed\n");
    }

    // We need to send the initial configure event. If we send it now, GTK will get it before it gets returned the
    // popup the configure is on, which obviously wont work. By creating a wl_callback we can bounce a message
    // back from the compositor and send configure when it arrives, which solves this problem. HOWEVER the GDK
    // surface is dispatching on a specific event queue. Our callback needs to be on the same queue, or else it
    // wont get processed and everything will lock up.
    struct wl_event_queue* queue = libwayland_shim_proxy_get_queue((struct wl_proxy*)self->super.xdg_surface);
    struct wl_display* display = libwayland_shim_proxy_get_display((struct wl_proxy*)self->super.xdg_surface);
    struct wl_display* wrapped_display = wl_proxy_create_wrapper(display);
    wl_proxy_set_queue((struct wl_proxy*)wrapped_display, queue);
    struct wl_callback* callback = wl_display_sync(wrapped_display);
    wl_callback_add_listener(callback, &stubbed_configure_callback_listener, self);
    wl_proxy_wrapper_destroy(wrapped_display);

    struct xdg_surface_server_t* parent_surface = get_xdg_surface_server_from_xdg_surface(super->popup_parent);
    if (parent_surface) {
        struct wl_subcompositor* subcompositor = get_subcompositor_global_from_display(display);
        assert(subcompositor);
        self->subsurface = wl_subcompositor_get_subsurface(
            subcompositor,
            super->wl_surface,
            parent_surface->wl_surface
        );
        wl_subsurface_set_desync(self->subsurface);
        wl_subsurface_set_position(self->subsurface, self->configure.top_left.x, self->configure.top_left.y);
        // Trigger parent commit to make subsurface appear
        xdg_surface_server_send_configure(
            parent_surface,
            parent_surface->last_configure.x, parent_surface->last_configure.y,
            parent_surface->last_configure.width, parent_surface->last_configure.height,
            0
        );
    }
}

static void stubbed_role_destroyed(struct xdg_surface_server_t* super) {
    struct stubbed_surface_t* self = (void*)super;

    if (self->subsurface) {
        wl_subsurface_destroy(self->subsurface);
    }
}

void stubbed_surface_destroyed(struct xdg_surface_server_t* super) {
    struct stubbed_surface_t* self = (void*)super;
    free(self);
}

struct wl_proxy* stubbed_surface_init(struct xdg_wm_base* creating_object, struct wl_surface* surface) {
    struct stubbed_surface_t* self = calloc(1, sizeof(struct stubbed_surface_t));
    self->super.popup_created = stubbed_role_created;
    self->super.toplevel_created = stubbed_role_created;
    self->super.popup_destroyed = stubbed_role_destroyed;
    self->super.toplevel_destroyed = stubbed_role_destroyed;
    return xdg_surface_server_get_xdg_surface(&self->super, creating_object, surface);
}

static bool xdg_positioner_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data, (void)opcode, (void)create_interface, (void)create_version, (void)flags, (void)ret_proxy;
    if (opcode == XDG_POSITIONER_DESTROY) return false;

    if (current_positioner.wayland_object != proxy) {
        memset(&current_positioner, 0, sizeof(current_positioner));
        current_positioner.wayland_object = proxy;
    }

    switch (opcode) {
        case XDG_POSITIONER_SET_SIZE:
            current_positioner.size = (struct geom_size_t){args[0].i, args[1].i};
            break;

        case XDG_POSITIONER_SET_ANCHOR_RECT:
            current_positioner.anchor_rect.top_left = (struct geom_point_t){args[0].i, args[1].i};
            current_positioner.anchor_rect.size = (struct geom_size_t){args[2].i, args[3].i};
            break;

        case XDG_POSITIONER_SET_ANCHOR:
            current_positioner.anchor = args[0].u;
            break;

        case XDG_POSITIONER_SET_GRAVITY:
            current_positioner.gravity = args[0].u;
            break;

        case XDG_POSITIONER_SET_CONSTRAINT_ADJUSTMENT:
            current_positioner.constraint_adjustment = args[0].u;
            break;

        case XDG_POSITIONER_SET_OFFSET:
            current_positioner.offset = (struct geom_point_t){args[0].i, args[1].i};
            break;
    }
    return false;
}

void stubbed_surface_install_positioner_hook() {
    static bool installed = false;
    if (!installed) {
        installed = true;
        libwayland_shim_install_request_hook(
            &xdg_positioner_interface,
            -1,
            xdg_positioner_hook,
            NULL
        );
    }
}
