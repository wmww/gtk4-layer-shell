#pragma once

#include <wayland-client.h>
#include <xdg-shell-client.h>

struct xdg_surface_server_t {
    // Virtual functions, can be null
    void (*window_geometry_set)(struct xdg_surface_server_t* super, int width, int height);
    void (*configure_acked)(struct xdg_surface_server_t* super, uint32_t serial);
    void (*toplevel_created)(struct xdg_surface_server_t* super);
    void (*toplevel_destroyed)(struct xdg_surface_server_t* super);
    void (*popup_created)(struct xdg_surface_server_t* super);
    void (*popup_destroyed)(struct xdg_surface_server_t* super);
    void (*surface_destroyed)(struct xdg_surface_server_t* super);

    struct wl_surface* wl_surface;
    struct xdg_surface* xdg_surface;
    struct xdg_surface* popup_parent;
    struct xdg_popup* xdg_popup;
    struct xdg_toplevel* xdg_toplevel;
    struct {
        int x, y, width, height;
    } last_configure;
};

// If the xdg_surface is managed by a surface server, this returns the surface server
struct xdg_surface_server_t* get_xdg_surface_server_from_xdg_surface(struct xdg_surface* source);

// Used in .get_popup to create a "stubbed" XDG popup, which is not displayed but shouldn't lock up GTK
struct wl_proxy* xdg_surface_server_get_xdg_surface(
    struct xdg_surface_server_t* self,
    struct xdg_wm_base* creating_object,
    struct wl_surface* surface
);

// x and y are only used for popups
void xdg_surface_server_send_configure(
    struct xdg_surface_server_t* self,
    int x, int y,
    int width, int height,
    uint32_t serial
);

// Simulates a destruction of the proxies. They are cleared, the *_destroyed virtual functions are called and any future
// requests made to them will be safely ignored. This object can be safely re-used.
void xdg_surface_server_uninit(struct xdg_surface_server_t* self);
