#include "layer-surface.h"

#include "registry.h"
#include "libwayland-shim.h"

#include "xdg-shell-client.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void layer_surface_send_set_size(struct layer_surface_t* self) {
    if (!self->layer_surface) return;

    struct geom_size_t size = self->last_xdg_window_geom_size;

    // Default values because setting 0 is only allowed when anchored
    if (size.width  <= 0) size.width  = 200;
    if (size.height <= 0) size.height = 200;

    if (self->anchored.left && self->anchored.right) {
        size.width = 0;
    }

    if (self->anchored.top && self->anchored.bottom) {
        size.height = 0;
    }

    if (self->cached_layer_size_set.width  != size.width ||
        self->cached_layer_size_set.height != size.height
    ) {
        self->cached_layer_size_set = size;
        zwlr_layer_surface_v1_set_size(self->layer_surface, size.width, size.height);
    }
}

static void layer_surface_configure_xdg_surface(
    struct layer_surface_t* self,
    uint32_t serial, // Can be 0
    bool send_even_if_size_unchanged
) {
    if (!self->has_initial_layer_shell_configure) {
        // skip sending xdg_toplevel and xdg_surface configure to the client program since layer shell surface has not
        // been configured yet and attaching a buffer to an unconfigured surface is a protocol error
        return;
    }

    struct geom_size_t size = {0, 0};

    if (self->anchored.left && self->anchored.right && self->last_layer_configured_size.width > 0) {
        size.width = self->last_layer_configured_size.width;
    }

    if (self->anchored.top && self->anchored.bottom && self->last_layer_configured_size.height > 0) {
        size.height = self->last_layer_configured_size.height;
    }

    // Ideally this wouldn't be needed, see the layer_surface_t.get_preferred_size documentation
    if (self->get_preferred_size && (size.width == 0 || size.height == 0)) {
        struct geom_size_t preferred = self->get_preferred_size(self);
        if (size.width == 0 && preferred.width > 0) {
            size.width = preferred.width;
        }
        if (size.height == 0 && preferred.height > 0) {
            size.height = preferred.height;
        }
    }

    if (self->cached_xdg_configure_size.width  != size.width ||
        self->cached_xdg_configure_size.height != size.height ||
        send_even_if_size_unchanged
    ) {
        self->cached_xdg_configure_size = size;
        if (serial) {
            self->pending_configure_serial = serial;
        }

        xdg_surface_server_send_configure(&self->super, 0, 0, size.width, size.height, serial);
    }
}

static void layer_surface_needs_commit(struct layer_surface_t* self) {
    // Send a configure to force the client program to commit the surface
    layer_surface_configure_xdg_surface(self, 0, true);
}

static void layer_surface_handle_configure(
    void* data,
    struct zwlr_layer_surface_v1* surface,
    uint32_t serial,
    uint32_t w,
    uint32_t h
) {
    (void)surface;
    struct layer_surface_t* self = data;
    self->last_layer_configured_size = (struct geom_size_t){w, h};
    self->has_initial_layer_shell_configure = true;
    layer_surface_configure_xdg_surface(self, serial, true);
}

static void layer_surface_handle_closed(void* data, struct zwlr_layer_surface_v1* _surface) {
    (void)_surface;
    struct layer_surface_t* self = data;
    if (self->respect_close && self->super.xdg_toplevel) {
        LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(xdg_toplevel_listener, close, self->super.xdg_toplevel);
    }
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_handle_configure,
    .closed = layer_surface_handle_closed,
};

static void layer_surface_send_set_anchor(struct layer_surface_t* self) {
    if (self->layer_surface) {
        zwlr_layer_surface_v1_set_anchor(
            self->layer_surface,
            (self->anchored.left   ? ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT   : 0) |
            (self->anchored.right  ? ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT  : 0) |
            (self->anchored.top    ? ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP    : 0) |
            (self->anchored.bottom ? ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM : 0)
        );
    }
}

static void layer_surface_send_set_margin(struct layer_surface_t* self) {
    if (self->layer_surface) {
        zwlr_layer_surface_v1_set_margin(
            self->layer_surface,
            self->margin_size.top,
            self->margin_size.right,
            self->margin_size.bottom,
            self->margin_size.left
        );
    }
}

static void layer_surface_send_set_keyboard_interactivity(struct layer_surface_t* self) {
    if (self->layer_surface) {
        enum zwlr_layer_surface_v1_keyboard_interactivity mode = self->keyboard_mode;
        if (mode == ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND) {
            uint32_t version = wl_proxy_get_version((struct wl_proxy*)self->layer_surface);
            if (version < ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND_SINCE_VERSION) {
                fprintf(
                    stderr,
                    "compositor uses layer shell version %d, which does not support on-demand keyboard interactivity\n",
                    version
                );
                mode = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;
            }
        }
        zwlr_layer_surface_v1_set_keyboard_interactivity(self->layer_surface, mode);
    }
}

static void layer_surface_create_surface_object(struct layer_surface_t* self, struct wl_surface* wl_surface) {
    struct wl_display* display = libwayland_shim_proxy_get_display((struct wl_proxy*)wl_surface);
    struct zwlr_layer_shell_v1* layer_shell_global = get_layer_shell_global_from_display(display);
    if (!layer_shell_global) {
        fprintf(stderr, "failed to create layer surface, no layer shell global\n");
        return;
    }

    const char* name_space = layer_surface_get_namespace(self);

    self->has_initial_layer_shell_configure = false;
    self->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        layer_shell_global,
        wl_surface,
        self->output,
        self->layer,
        name_space);
    assert(self->layer_surface);
    zwlr_layer_surface_v1_add_listener(self->layer_surface, &layer_surface_listener, self);

    layer_surface_send_set_keyboard_interactivity(self);
    zwlr_layer_surface_v1_set_exclusive_zone(self->layer_surface, self->exclusive_zone);
    layer_surface_send_set_anchor(self);
    layer_surface_send_set_margin(self);
    layer_surface_send_set_size(self);
}

static void layer_surface_update_auto_exclusive_zone(struct layer_surface_t* self) {
    if (!self->auto_exclusive_zone) return;

    bool horiz = (self->anchored.left == self->anchored.right);
    bool vert  = (self->anchored.top  == self->anchored.bottom);
    int new_exclusive_zone = -1;

    if (horiz && !vert) {
        new_exclusive_zone = self->last_xdg_window_geom_size.height;
        if (!self->anchored.top) {
            new_exclusive_zone += self->margin_size.top;
        }
        if (!self->anchored.bottom) {
            new_exclusive_zone += self->margin_size.bottom;
        }
    } else if (vert && !horiz) {
        new_exclusive_zone = self->last_xdg_window_geom_size.width;
        if (!self->anchored.left) {
            new_exclusive_zone += self->margin_size.left;
        }
        if (!self->anchored.right) {
            new_exclusive_zone += self->margin_size.right;
        }
    }

    if (new_exclusive_zone >= 0 && self->exclusive_zone != new_exclusive_zone) {
        self->exclusive_zone = new_exclusive_zone;
        if (self->layer_surface) {
            zwlr_layer_surface_v1_set_exclusive_zone(self->layer_surface, self->exclusive_zone);
        }
    }
}

static void layer_surface_window_geometry_set(struct xdg_surface_server_t* super, int width, int height) {
    struct layer_surface_t* self = (void*)super;

    self->last_xdg_window_geom_size = (struct geom_size_t){width, height};
    layer_surface_send_set_size(self);
    layer_surface_update_auto_exclusive_zone(self);
}

void layer_surface_configure_acked(struct xdg_surface_server_t* super, uint32_t serial) {
    struct layer_surface_t* self = (void*)super;

    if (serial && serial == self->pending_configure_serial) {
        self->pending_configure_serial = 0;
        zwlr_layer_surface_v1_ack_configure(self->layer_surface, serial);
    }
}

static void layer_surface_role_destroyed(struct xdg_surface_server_t* super) {
    struct layer_surface_t* self = (void*)super;

    if (self->layer_surface) {
        zwlr_layer_surface_v1_destroy(self->layer_surface);
        self->layer_surface = NULL;
    }

    self->cached_xdg_configure_size = GEOM_SIZE_UNSET;
    self->last_xdg_window_geom_size = GEOM_SIZE_UNSET;
    self->cached_layer_size_set = GEOM_SIZE_UNSET;
    self->last_layer_configured_size = GEOM_SIZE_UNSET;

    self->pending_configure_serial = 0;
    self->has_initial_layer_shell_configure = false;
}

struct layer_surface_t layer_surface_make() {
    struct layer_surface_t ret = {
        .super = {
            .window_geometry_set = layer_surface_window_geometry_set,
            .configure_acked = layer_surface_configure_acked,
            .toplevel_destroyed = layer_surface_role_destroyed,
            .popup_destroyed = layer_surface_role_destroyed,
        },
        .cached_xdg_configure_size = GEOM_SIZE_UNSET,
        .last_xdg_window_geom_size = GEOM_SIZE_UNSET,
        .cached_layer_size_set = GEOM_SIZE_UNSET,
        .last_layer_configured_size = GEOM_SIZE_UNSET,
        .has_initial_layer_shell_configure = false,
        .keyboard_mode = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE,
        .layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP,
    };
    return ret;
}

void layer_surface_uninit(struct layer_surface_t* self) {
    xdg_surface_server_uninit(&self->super);
    free((void*)self->name_space);
}

void layer_surface_set_output(struct layer_surface_t* self, struct wl_output* output) {
    if (self->output != output) {
        self->output = output;
        if (self->layer_surface && self->remap) {
            self->remap(self);
        }
    }
}

void layer_surface_set_name_space(struct layer_surface_t* self, char const* name_space) {
    if (strcmp(self->name_space ? self->name_space : "", name_space ? name_space : "") != 0) {
        free((void*)self->name_space);
        self->name_space = (name_space && *name_space) ? strdup(name_space) : NULL;
        if (self->layer_surface && self->remap) {
            self->remap(self);
        }
    }
}

const char* layer_surface_get_namespace(struct layer_surface_t* self) {
    if (self && self->name_space) {
        return self->name_space;
    } else {
        return "gtk4-layer-shell";
    }
}

void layer_surface_set_layer(struct layer_surface_t* self, enum zwlr_layer_shell_v1_layer layer) {
    if (self->layer != layer) {
        self->layer = layer;
        if (self->layer_surface) {
            uint32_t version = zwlr_layer_surface_v1_get_version(self->layer_surface);
            if (version >= ZWLR_LAYER_SURFACE_V1_SET_LAYER_SINCE_VERSION) {
                zwlr_layer_surface_v1_set_layer(self->layer_surface, layer);
                layer_surface_needs_commit(self);
            } else if (self->remap) {
                self->remap(self);
            }
        }
    }
}

void layer_surface_set_anchor(struct layer_surface_t* self, struct geom_edges_t anchors) {
    // Normalize booleans or else strange things can happen
    anchors.left   = anchors.left   ? 1 : 0;
    anchors.right  = anchors.right  ? 1 : 0;
    anchors.top    = anchors.top    ? 1 : 0;
    anchors.bottom = anchors.bottom ? 1 : 0;

    if (self->anchored.left   != anchors.left ||
        self->anchored.right  != anchors.right ||
        self->anchored.top    != anchors.top   ||
        self->anchored.bottom != anchors.bottom
    ) {
        self->anchored = anchors;
        if (self->layer_surface) {
            layer_surface_send_set_anchor(self);
            layer_surface_send_set_size(self);
            layer_surface_configure_xdg_surface(self, 0, false);
            layer_surface_update_auto_exclusive_zone(self);
            layer_surface_needs_commit(self);
        }
    }
}

void layer_surface_set_margin(struct layer_surface_t* self, struct geom_edges_t margins) {
    if (self->margin_size.left   != margins.left ||
        self->margin_size.right  != margins.right ||
        self->margin_size.top    != margins.top   ||
        self->margin_size.bottom != margins.bottom
    ) {
        self->margin_size = margins;
        layer_surface_send_set_margin(self);
        layer_surface_update_auto_exclusive_zone(self);
        layer_surface_needs_commit(self);
    }
}

void layer_surface_set_exclusive_zone(struct layer_surface_t* self, int exclusive_zone) {
    self->auto_exclusive_zone = false;
    if (exclusive_zone < -1) {
        exclusive_zone = -1;
    }
    if (self->exclusive_zone != exclusive_zone) {
        self->exclusive_zone = exclusive_zone;
        if (self->layer_surface) {
            zwlr_layer_surface_v1_set_exclusive_zone(self->layer_surface, self->exclusive_zone);
            layer_surface_needs_commit(self);
        }
    }
}

void layer_surface_auto_exclusive_zone_enable(struct layer_surface_t* self) {
    if (!self->auto_exclusive_zone) {
        self->auto_exclusive_zone = true;
        layer_surface_update_auto_exclusive_zone(self);
    }
}

void layer_surface_set_keyboard_mode(struct layer_surface_t* self, enum zwlr_layer_surface_v1_keyboard_interactivity mode) {
    if (self->keyboard_mode != mode) {
        self->keyboard_mode = mode;
        if (self->layer_surface) {
            layer_surface_send_set_keyboard_interactivity(self);
            layer_surface_needs_commit(self);
        }
    }
}

void layer_surface_invalidate_preferred_size(struct layer_surface_t* self) {
    layer_surface_configure_xdg_surface(self, 0, false);
}

void layer_surface_set_respect_close(struct layer_surface_t* self, bool respect_close) {
    self->respect_close = respect_close;
}

static bool xdg_wm_base_get_xdg_surface_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data;
    (void)opcode;
    (void)create_interface;
    (void)create_version;
    (void)flags;

    layer_surface_hook_callback_t callback = data;
    struct wl_surface* wl_surface = (struct wl_surface*)args[1].o;
    struct layer_surface_t* self = callback(wl_surface);

    if (self) {
        *ret_proxy = xdg_surface_server_get_xdg_surface(&self->super, (struct xdg_wm_base*)proxy, wl_surface);
        layer_surface_create_surface_object(self, wl_surface);
        return true;
    } else {
        return false;
    }
}

static bool xdg_surface_get_popup_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data;
    (void)opcode;
    (void)create_interface;
    (void)create_version;
    (void)flags;

    struct xdg_surface* parent_xdg_surface = (struct xdg_surface*)args[1].o;
    struct xdg_surface_server_t* super = get_xdg_surface_server_from_xdg_surface(parent_xdg_surface);
    // If super is non-null it's a valid xdg_surface_server_t, but we don't know it's part of a layer_surface_t. Check
    // to make sure one of its virtual functions match before casting it (basically a dynamic_case<>).
    struct layer_surface_t* self =
        super && super->configure_acked == layer_surface_configure_acked ?
        (void*)super : NULL;

    if (self) {
        if (self->layer_surface) {
            struct xdg_surface* popup_xdg_surface = (struct xdg_surface*)proxy;
            struct xdg_positioner* positioner = (struct xdg_positioner*)args[2].o;
            struct xdg_popup* xdg_popup = xdg_surface_get_popup(popup_xdg_surface, NULL, positioner);
            zwlr_layer_surface_v1_get_popup(self->layer_surface, xdg_popup);
            *ret_proxy = (struct wl_proxy*)xdg_popup;
            return true;
        } else {
            fprintf(stderr, "tried to create popup before layer shell surface\n");;
        }
    }
    return false;
}

void layer_surface_install_hook(layer_surface_hook_callback_t callback) {
    libwayland_shim_install_request_hook(
        &xdg_wm_base_interface,
        XDG_WM_BASE_GET_XDG_SURFACE,
        xdg_wm_base_get_xdg_surface_hook,
        callback
    );

    static bool popup_hook_initialized = false;
    if (!popup_hook_initialized) {
        popup_hook_initialized = true;
        libwayland_shim_install_request_hook(
            &xdg_surface_interface,
            XDG_SURFACE_GET_POPUP,
            xdg_surface_get_popup_hook,
            NULL
        );
    }
}
