#include "xdg-surface-server.h"
#include "libwayland-shim.h"

#include <assert.h>

static bool xdg_popup_handle_request(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data, (void)proxy, (void)opcode, (void)create_interface, (void)create_version, (void)flags, (void)args, (void)ret_proxy;
    return false;
}

static void xdg_popup_handle_destroy(void* data, struct wl_proxy* proxy) {
    (void)proxy;
    struct xdg_surface_server_t* self = data;
    self->xdg_popup = NULL;
    if (self->popup_destroyed) self->popup_destroyed(self);
}

static bool xdg_toplevel_handle_request(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data, (void)proxy, (void)opcode, (void)create_interface, (void)create_version, (void)flags, (void)args, (void)ret_proxy;
    return false;
}

static void xdg_toplevel_handle_destroy(void* data, struct wl_proxy* proxy) {
    (void)proxy;
    struct xdg_surface_server_t* self = data;
    self->xdg_toplevel = NULL;
    if (self->toplevel_destroyed) self->toplevel_destroyed(self);
}

static bool xdg_surface_handle_request(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)proxy, (void)create_interface, (void)create_version, (void)flags;
    struct xdg_surface_server_t* self = data;
    switch (opcode) {
        case XDG_SURFACE_GET_TOPLEVEL:
            // Make sure surface doesn't already have a role
            assert(!self->xdg_toplevel);
            assert(!self->xdg_popup);
            *ret_proxy = libwayland_shim_create_client_proxy(
                proxy,
                &xdg_toplevel_interface,
                create_version,
                xdg_toplevel_handle_request,
                xdg_toplevel_handle_destroy,
                self
            );
            self->xdg_toplevel = (struct xdg_toplevel*)*ret_proxy;
            if (self->toplevel_created) self->toplevel_created(self);
            return true;

        case XDG_SURFACE_GET_POPUP:
            // Make sure surface doesn't already have a role
            assert(!self->xdg_toplevel);
            assert(!self->xdg_popup);
            *ret_proxy = libwayland_shim_create_client_proxy(
                proxy,
                &xdg_popup_interface,
                create_version,
                xdg_popup_handle_request,
                xdg_popup_handle_destroy,
                self
            );
            self->xdg_popup = (struct xdg_popup*)*ret_proxy;
            if (self->popup_created) self->popup_created(self);
            return true;

        case XDG_SURFACE_SET_WINDOW_GEOMETRY:
            if (self->window_geometry_set) self->window_geometry_set(self, args[2].i, args[3].i);
            return true;

        case XDG_SURFACE_ACK_CONFIGURE:
            if (self->configure_acked) self->configure_acked(self, args[0].u);
            return true;

        default: return false;
    }
}

static void xdg_surface_handle_destroy(void* data, struct wl_proxy* proxy) {
    (void)proxy;
    struct xdg_surface_server_t* self = data;
    self->xdg_surface = NULL;
    if (self->surface_destroyed) self->surface_destroyed(self);
    self->wl_surface = NULL;
}

struct wl_proxy* xdg_surface_server_get_xdg_surface(
    struct xdg_surface_server_t* self,
    struct xdg_wm_base* creating_object,
    struct wl_surface* surface
) {
    assert(!self->xdg_surface);
    self->wl_surface = surface;
    struct wl_proxy* xdg_surface = libwayland_shim_create_client_proxy(
        (struct wl_proxy*)creating_object,
        &xdg_surface_interface,
        1, // XDG shell v1 is simpler to implement, for example this stops GTK from trying to reposition popups
        xdg_surface_handle_request,
        xdg_surface_handle_destroy,
        self
    );
    self->xdg_surface = (struct xdg_surface*)xdg_surface;
    return xdg_surface;
}

void xdg_surface_server_send_configure(
    struct xdg_surface_server_t* self,
    int x, int y,
    int width, int height,
    uint32_t serial
) {
    if (self->xdg_toplevel) {
        struct wl_array states;
        wl_array_init(&states);
        {
            uint32_t* state = wl_array_add(&states, sizeof(uint32_t));
            assert(state);
            *state = XDG_TOPLEVEL_STATE_ACTIVATED;
        }{
            uint32_t* state = wl_array_add(&states, sizeof(uint32_t));
            assert(state);
            *state = XDG_TOPLEVEL_STATE_FULLSCREEN;
        }

        LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
            xdg_toplevel_listener,
            configure,
            self->xdg_toplevel,
            width, height,
            &states
        );

        wl_array_release(&states);
    }

    if (self->xdg_popup) {
        LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
            xdg_popup_listener,
            configure,
            self->xdg_popup,
            x, y,
            width, height
        );
    }

    if (self->xdg_surface) {
        LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
            xdg_surface_listener,
            configure,
            self->xdg_surface,
            serial
        );
    }
}
