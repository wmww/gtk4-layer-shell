#include "layer-surface.h"

#include "wayland-utils.h"
#include "libwayland-shim.h"

#include "wlr-layer-shell-unstable-v1-client.h"
#include "xdg-shell-client.h"

#include <gtk/gtk.h>
#include <gdk/wayland/gdkwayland.h>

static const char* layer_surface_key = "wayland_layer_surface";
GList* all_layer_surfaces = NULL;

LayerSurface* gtk_window_get_layer_surface(GtkWindow* gtk_window) {
    if (!gtk_window) return NULL;
    return g_object_get_data(G_OBJECT(gtk_window), layer_surface_key);
}

static void layer_surface_remap(LayerSurface* self) {
    gtk_widget_unrealize(GTK_WIDGET(self->gtk_window));
    gtk_widget_map(GTK_WIDGET(self->gtk_window));
}

static void layer_surface_get_preferred_size(LayerSurface* self, int* width, int* height) {
    gtk_window_get_default_size(self->gtk_window, width, height);

    GtkRequisition natural;
    gtk_widget_get_preferred_size(GTK_WIDGET(self->gtk_window), NULL, &natural);

    if (!*width)  *width  = natural.width;
    if (!*height) *height = natural.height;
}

static void layer_surface_send_set_size(LayerSurface* self) {
    g_return_if_fail(self->layer_surface);

    int width  = gtk_widget_get_width (GTK_WIDGET(self->gtk_window));
    int height = gtk_widget_get_height(GTK_WIDGET(self->gtk_window));
    if (!width || !height) {
        layer_surface_get_preferred_size(self, &width, &height);
    }

    if (self->anchored.left && self->anchored.right) {
        width = 0;
    }

    if (self->anchored.top && self->anchored.bottom) {
        height = 0;
    }

    if (self->cached_layer_size_set.width  != width ||
        self->cached_layer_size_set.height != height
    ) {
        self->cached_layer_size_set = (struct geom_size_t){width, height};
        zwlr_layer_surface_v1_set_size(self->layer_surface, width, height);
    }
}

static void layer_surface_configure_xdg_surface(
    LayerSurface* self,
    uint32_t serial,
    gboolean send_even_if_size_unchanged
) {
    if (!self->client_facing_xdg_surface || !self->client_facing_xdg_toplevel) {
        return;
    }

    int width, height;
    layer_surface_get_preferred_size(self, &width, &height);

    if (self->anchored.left && self->anchored.right && self->last_layer_configured_size.width) {
        width = self->last_layer_configured_size.width;
    }

    if (self->anchored.top && self->anchored.bottom && self->last_layer_configured_size.height) {
        height = self->last_layer_configured_size.height;
    }

    if (!self->has_initial_layer_shell_configure) {
        // skip sending xdg_toplevel and xdg_surface configure to GTK
        // since layer shell surface has not been configured yet and
        // attaching a buffer to an unconfigured surface is a protocol error
        return;
    }

    if (self->cached_xdg_configure_size.width != width ||
        self->cached_xdg_configure_size.height != height ||
        send_even_if_size_unchanged
    ) {
        self->cached_xdg_configure_size = (struct geom_size_t){width, height};
        if (serial) {
            self->pending_configure_serial = serial;
        }

        struct wl_array states;
        wl_array_init(&states); {
            uint32_t* state = wl_array_add(&states, sizeof(uint32_t));
            g_assert(state);
            *state = XDG_TOPLEVEL_STATE_ACTIVATED;
        } {
            uint32_t* state = wl_array_add(&states, sizeof(uint32_t));
            g_assert(state);
            *state = XDG_TOPLEVEL_STATE_MAXIMIZED;
        }

        LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
            xdg_toplevel_listener,
            configure,
            self->client_facing_xdg_toplevel,
            width, height,
            &states);
        wl_array_release(&states);

        LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
            xdg_surface_listener,
            configure,
            self->client_facing_xdg_surface,
            serial);
    }
}

static void layer_surface_needs_commit(LayerSurface* self) {
    // Send a configure to force GTK to commit the surface
    layer_surface_configure_xdg_surface(self, 0, TRUE);
}

static void layer_surface_handle_configure(
    void* data,
    struct zwlr_layer_surface_v1* surface,
    uint32_t serial,
    uint32_t w,
    uint32_t h
) {
    (void)surface;
    LayerSurface* self = data;
    self->last_layer_configured_size = (struct geom_size_t){w, h};
    self->has_initial_layer_shell_configure = true;
    layer_surface_configure_xdg_surface(self, serial, TRUE);
}

static void layer_surface_handle_closed(void* data, struct zwlr_layer_surface_v1* _surface) {
    (void)_surface;
    LayerSurface* self = data;
    gtk_window_close(self->gtk_window);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_handle_configure,
    .closed = layer_surface_handle_closed,
};

static void layer_surface_send_set_anchor(LayerSurface* self) {
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

static void layer_surface_send_set_margin(LayerSurface* self) {
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

static void layer_surface_create_surface_object(LayerSurface* self, struct wl_surface* wl_surface) {
    struct zwlr_layer_shell_v1* layer_shell_global = gtk_wayland_get_layer_shell_global();
    g_return_if_fail(layer_shell_global);

    const char* name_space = layer_surface_get_namespace(self);

    struct wl_output* output = NULL;
    if (self->monitor) {
        output = gdk_wayland_monitor_get_wl_output(self->monitor);
    }

    self->has_initial_layer_shell_configure = false;
    self->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        layer_shell_global,
        wl_surface,
        output,
        self->layer,
        name_space);
    g_return_if_fail(self->layer_surface);
    zwlr_layer_surface_v1_add_listener(self->layer_surface, &layer_surface_listener, self);

    zwlr_layer_surface_v1_set_keyboard_interactivity(self->layer_surface, self->keyboard_mode);
    zwlr_layer_surface_v1_set_exclusive_zone(self->layer_surface, self->exclusive_zone);
    layer_surface_send_set_anchor(self);
    layer_surface_send_set_margin(self);
    layer_surface_send_set_size(self);
}

static void layer_surface_unmap(LayerSurface* super) {
    LayerSurface* self = (LayerSurface*)super;

    if (self->layer_surface) {
        zwlr_layer_surface_v1_destroy(self->layer_surface);
        self->layer_surface = NULL;
    }

    libwayland_shim_clear_client_proxy_data((struct wl_proxy*)self->client_facing_xdg_surface);
    libwayland_shim_clear_client_proxy_data((struct wl_proxy*)self->client_facing_xdg_toplevel);

    self->client_facing_xdg_surface = NULL;
    self->client_facing_xdg_toplevel = NULL;
    self->has_initial_layer_shell_configure = false;

    self->cached_xdg_configure_size = (struct geom_size_t){-1, -1};
    self->cached_layer_size_set = (struct geom_size_t){-1, -1};
    self->last_layer_configured_size = (struct geom_size_t){0, 0};
    self->pending_configure_serial = 0;
}

static void layer_surface_destroy(LayerSurface* self) {
    layer_surface_unmap(self);
    all_layer_surfaces = g_list_remove(all_layer_surfaces, self);
    g_free((gpointer)self->name_space);
    g_free(self);

}

static void layer_surface_update_auto_exclusive_zone(LayerSurface* self) {
    if (!self->auto_exclusive_zone) return;

    gboolean horiz = (self->anchored.left == self->anchored.right);
    gboolean vert  = (self->anchored.top  == self->anchored.bottom);
    int new_exclusive_zone = -1;

    int window_width  = gtk_widget_get_width(GTK_WIDGET(self->gtk_window));
    int window_height = gtk_widget_get_height(GTK_WIDGET(self->gtk_window));
    if (horiz && !vert) {
        new_exclusive_zone = window_height;
        if (!self->anchored.top) {
            new_exclusive_zone += self->margin_size.top;
        }
        if (!self->anchored.bottom) {
            new_exclusive_zone += self->margin_size.bottom;
        }
    } else if (vert && !horiz) {
        new_exclusive_zone = window_width;
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

static void layer_surface_on_default_size_set(GtkWindow* _window, const GParamSpec* _pspec, LayerSurface* self) {
    (void)_window;
    (void)_pspec;
    layer_surface_configure_xdg_surface(self, 0, FALSE);
}

LayerSurface* layer_surface_new(GtkWindow* gtk_window) {
    if (!GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        g_warning("Failed to initialize layer surface, not on Wayland");
        return NULL;
    }

    if (!libwayland_shim_has_initialized()) {
        g_warning("Failed to initialize layer surface, GTK4 Layer Shell may have been linked after libwayland.");
        g_message("Move gtk4-layer-shell before libwayland-client in the linker options.");
        g_message("You may be able to fix with without recompiling by setting LD_PRELOAD=/path/to/libgtk4-layer-shell.so");
        g_message("See https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md for more info");
        return NULL;
    }

    if (!gtk_wayland_get_layer_shell_global()) {
        g_warning("Failed to initialize layer surface, it appears your Wayland compositor doesn't support Layer Shell");
        return NULL;
    }

    if (!gtk_window) {
        g_warning("Failed to initialize layer surface, provided window is null");
        return NULL;
    }

    LayerSurface* self = g_new0(LayerSurface, 1);
    self->cached_xdg_configure_size = (struct geom_size_t){-1, -1};
    self->cached_layer_size_set = (struct geom_size_t){-1, -1};
    self->has_initial_layer_shell_configure = false;
    all_layer_surfaces = g_list_append(all_layer_surfaces, self);

    self->gtk_window = gtk_window;

    g_object_set_data_full(G_OBJECT(gtk_window), layer_surface_key, self, (GDestroyNotify)layer_surface_destroy);

    self->layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    self->keyboard_mode = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;

    gtk_window_set_decorated(gtk_window, FALSE);
    g_signal_connect(gtk_window, "notify::default-width", G_CALLBACK(layer_surface_on_default_size_set), self);
    g_signal_connect(gtk_window, "notify::default-height", G_CALLBACK(layer_surface_on_default_size_set), self);

    if (gtk_widget_get_mapped(GTK_WIDGET(gtk_window))) {
        layer_surface_remap(self);
    }

    return self;
}

void layer_surface_set_monitor(LayerSurface* self, GdkMonitor* monitor) {
    if (monitor) {
        g_return_if_fail(GDK_IS_WAYLAND_MONITOR(monitor));
    }
    if (monitor != self->monitor) {
        self->monitor = monitor;
        if (self->layer_surface) {
            layer_surface_remap(self);
        }
    }
}

void layer_surface_set_name_space(LayerSurface* self, char const* name_space) {
    if (g_strcmp0(self->name_space, name_space) != 0) {
        g_free((gpointer)self->name_space);
        self->name_space = g_strdup(name_space);
        if (self->layer_surface) {
            layer_surface_remap(self);
        }
    }
}

void layer_surface_set_layer(LayerSurface* self, enum zwlr_layer_shell_v1_layer layer) {
    if (self->layer != layer) {
        self->layer = layer;
        if (self->layer_surface) {
            uint32_t version = zwlr_layer_surface_v1_get_version(self->layer_surface);
            if (version >= ZWLR_LAYER_SURFACE_V1_SET_LAYER_SINCE_VERSION) {
                zwlr_layer_surface_v1_set_layer(self->layer_surface, layer);
                layer_surface_needs_commit(self);
            } else {
                layer_surface_remap(self);
            }
        }
    }
}

void layer_surface_set_anchor(LayerSurface* self, struct geom_edges_t anchors) {
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
            layer_surface_configure_xdg_surface(self, 0, FALSE);
            layer_surface_update_auto_exclusive_zone(self);
            layer_surface_needs_commit(self);
        }
    }
}

void layer_surface_set_margin(LayerSurface* self, struct geom_edges_t margins) {
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

void layer_surface_set_exclusive_zone(LayerSurface* self, int exclusive_zone) {
    self->auto_exclusive_zone = FALSE;
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

void layer_surface_auto_exclusive_zone_enable(LayerSurface* self) {
    if (!self->auto_exclusive_zone) {
        self->auto_exclusive_zone = TRUE;
        layer_surface_update_auto_exclusive_zone(self);
    }
}

void layer_surface_set_keyboard_mode(LayerSurface* self, enum zwlr_layer_surface_v1_keyboard_interactivity mode) {
    if (mode == ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND) {
        uint32_t version = gtk_layer_get_protocol_version();
        if (version < ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND_SINCE_VERSION) {
            g_warning(
                "Compositor uses layer shell version %d, which does not support on-demand keyboard interactivity",
                version);
            mode = ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;
        }
    }
    if (self->keyboard_mode != mode) {
        self->keyboard_mode = mode;
        if (self->layer_surface) {
            zwlr_layer_surface_v1_set_keyboard_interactivity(self->layer_surface, self->keyboard_mode);
            layer_surface_needs_commit(self);
        }
    }
}

const char* layer_surface_get_namespace(LayerSurface* self) {
    if (self && self->name_space) {
        return self->name_space;
    } else {
        return "gtk4-layer-shell";
    }
}

static void stubbed_xdg_toplevel_handle_destroy(void* data, struct wl_proxy* proxy) {
    (void)proxy;
    LayerSurface* self = (LayerSurface*)data;
    layer_surface_unmap(self);
}

static bool stubbed_xdg_surface_handle_request(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)interface; (void)flags;
    LayerSurface* self = (LayerSurface*)data;
    if (opcode == XDG_SURFACE_GET_TOPLEVEL) {
        *ret_proxy = libwayland_shim_create_client_proxy(
            proxy,
            &xdg_toplevel_interface,
            version,
            NULL,
            stubbed_xdg_toplevel_handle_destroy,
            data
        );
        self->client_facing_xdg_toplevel = (struct xdg_toplevel*)*ret_proxy;
        return true;
    } else if (opcode == XDG_SURFACE_GET_POPUP) {
        g_critical("XDG surface intercepted, but is now being used as popup");
        *ret_proxy = libwayland_shim_create_client_proxy(proxy, &xdg_popup_interface, version, NULL, NULL, NULL);
        return true;
    } else if (opcode == XDG_SURFACE_SET_WINDOW_GEOMETRY) {
        layer_surface_send_set_size(self);
        layer_surface_update_auto_exclusive_zone(self);
        return false;
    } else if (opcode == XDG_SURFACE_ACK_CONFIGURE) {
        uint32_t serial = args[0].u;
        if (serial && serial == self->pending_configure_serial) {
            self->pending_configure_serial = 0;
            zwlr_layer_surface_v1_ack_configure(self->layer_surface, serial);
        }
        return false;
    } else {
        return false;
    }
}

static void stubbed_xdg_surface_handle_destroy(void* data, struct wl_proxy* proxy) {
    (void)proxy;
    LayerSurface* self = (LayerSurface*)data;
    layer_surface_unmap(self);
}

gint find_layer_surface_with_wl_surface(gconstpointer layer_surface, gconstpointer needle) {
    const LayerSurface* self = layer_surface;
    GdkSurface* gdk_surface = gtk_native_get_surface(GTK_NATIVE(self->gtk_window));
    if (!gdk_surface) return 1;
    struct wl_surface* wl_surface = gdk_wayland_surface_get_wl_surface(gdk_surface);
    return wl_surface == needle ? 0 : 1;
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
    (void)flags;

    struct wl_surface* wl_surface = (struct wl_surface*)args[1].o;
    GList* layer_surface_entry = g_list_find_custom(
        all_layer_surfaces,
        wl_surface,
        find_layer_surface_with_wl_surface
    );

    if (layer_surface_entry) {
        LayerSurface* self = layer_surface_entry->data;
        struct wl_proxy* xdg_surface = libwayland_shim_create_client_proxy(
            proxy,
            &xdg_surface_interface,
            create_version,
            stubbed_xdg_surface_handle_request,
            stubbed_xdg_surface_handle_destroy,
            self
        );
        self->client_facing_xdg_surface = (struct xdg_surface*)xdg_surface;
        layer_surface_create_surface_object(self, wl_surface);
        *ret_proxy = xdg_surface;
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
    (void)flags;

    LayerSurface* self = libwayland_shim_get_client_proxy_data(
        (struct wl_proxy*)args[1].o,
        stubbed_xdg_surface_handle_request
    );

    if (self) {
        if (self->layer_surface) {
            struct xdg_popup* xdg_popup = xdg_surface_get_popup(
                (struct xdg_surface*)proxy,
                NULL,
                (struct xdg_positioner*)args[2].o
            );
            zwlr_layer_surface_v1_get_popup(self->layer_surface, xdg_popup);
            *ret_proxy = (struct wl_proxy*)xdg_popup;
        } else {
            g_critical("tried to create popup before layer shell surface");
            *ret_proxy = libwayland_shim_create_client_proxy(
                proxy,
                &xdg_popup_interface,
                create_version,
                NULL, NULL, NULL
            );
        }
        return true;
    } else {
        return false;
    }
}

__attribute__((constructor))
static void init_hooks() {
    libwayland_shim_install_request_hook(
        &xdg_wm_base_interface,
        XDG_WM_BASE_GET_XDG_SURFACE,
        xdg_wm_base_get_xdg_surface_hook,
        NULL
    );
    libwayland_shim_install_request_hook(
        &xdg_surface_interface,
        XDG_SURFACE_GET_POPUP,
        xdg_surface_get_popup_hook,
        NULL
    );
}
