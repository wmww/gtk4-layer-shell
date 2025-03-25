#include "gtk4-layer-shell.h"
#include "registry.h"
#include "layer-surface.h"
#include "libwayland-shim.h"

#include <gdk/wayland/gdkwayland.h>

static const char* layer_surface_key = "wayland_layer_surface";
static GList* all_layer_surfaces = NULL;
static GListModel* gtk_monitors = NULL;

struct gtk_layer_surface_t {
    struct layer_surface_t super;
    GtkWindow* gtk_window;
    GdkMonitor* monitor;
};

static void gtk_layer_surface_remap(struct layer_surface_t* super);
static void gtk_layer_surface_clear_monitor(struct gtk_layer_surface_t* layer_surface);

GTK4_LAYER_SHELL_EXPORT
guint gtk_layer_get_major_version() {
    return GTK_LAYER_SHELL_MAJOR;
}

GTK4_LAYER_SHELL_EXPORT
guint gtk_layer_get_minor_version() {
    return GTK_LAYER_SHELL_MINOR;
}

GTK4_LAYER_SHELL_EXPORT
guint gtk_layer_get_micro_version() {
    return GTK_LAYER_SHELL_MICRO;
}

static void monitors_changed(
    GListModel* self,
    guint position,
    guint removed,
    guint added,
    gpointer user_data
) {
    (void)self; (void)position; (void)removed; (void)added; (void)user_data;
    for (GList* item = all_layer_surfaces; item; item = item->next) {
        struct gtk_layer_surface_t* surface = item->data;
        if (surface->monitor == NULL) {
            // If the surface has a monitor set, it is in charge of responding to monitor changes
            gtk_layer_surface_remap(&surface->super);
        }
    }
}

static struct zwlr_layer_shell_v1* init_and_get_layer_shell_global() {
    gtk_init();
    GdkDisplay* gdk_display = gdk_display_get_default();
    g_return_val_if_fail(gdk_display, NULL);
    if (!gtk_monitors) {
        gtk_monitors = gdk_display_get_monitors(gdk_display);
        g_signal_connect(gtk_monitors, "items-changed",  G_CALLBACK(monitors_changed), NULL);
    }
    g_return_val_if_fail(GDK_IS_WAYLAND_DISPLAY(gdk_display), NULL);
    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);
    return get_layer_shell_global_from_display(wl_display);
}

GTK4_LAYER_SHELL_EXPORT
gboolean gtk_layer_is_supported() {
    return init_and_get_layer_shell_global() != NULL && libwayland_shim_has_initialized();
}

GTK4_LAYER_SHELL_EXPORT
guint gtk_layer_get_protocol_version() {
    struct zwlr_layer_shell_v1* layer_shell_global = init_and_get_layer_shell_global();
    if (!layer_shell_global) {
        return 0;
    }
    return zwlr_layer_shell_v1_get_version(layer_shell_global);
}

static struct gtk_layer_surface_t* gtk_window_get_layer_surface(GtkWindow* gtk_window) {
    if (!gtk_window) return NULL;
    return g_object_get_data(G_OBJECT(gtk_window), layer_surface_key);
}

static struct gtk_layer_surface_t* gtk_window_get_layer_surface_or_warn(GtkWindow* window) {
    g_return_val_if_fail(window, NULL);
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface(window);
    if (!layer_surface) {
        g_warning("GtkWindow is not a layer surface. Make sure you called gtk_layer_init_for_window()");
        return NULL;
    }
    return layer_surface;
}

static void gtk_layer_surface_destroy(struct gtk_layer_surface_t* self) {
    gtk_layer_surface_clear_monitor(self);
    layer_surface_uninit(&self->super);
    all_layer_surfaces = g_list_remove(all_layer_surfaces, self);
    g_free(self);
}

static gint find_layer_surface_with_wl_surface(gconstpointer layer_surface, gconstpointer needle) {
    struct gtk_layer_surface_t* self = (struct gtk_layer_surface_t*)layer_surface;
    GdkSurface* gdk_surface = gtk_native_get_surface(GTK_NATIVE(self->gtk_window));
    if (!gdk_surface) return 1;
    struct wl_surface* wl_surface = gdk_wayland_surface_get_wl_surface(gdk_surface);
    return wl_surface == needle ? 0 : 1;
}

static struct layer_surface_t* layer_surface_hook_callback_impl(struct wl_surface* wl_surface) {
    GList* entry = g_list_find_custom(
        all_layer_surfaces,
        wl_surface,
        find_layer_surface_with_wl_surface
    );
    return entry ? entry->data : NULL;
}

static struct geom_size_t gtk_layer_surface_get_preferred_size(struct layer_surface_t* super) {
    struct gtk_layer_surface_t* self = (struct gtk_layer_surface_t*)super;
    struct geom_size_t size = {0};
    gtk_window_get_default_size(self->gtk_window, &size.width, &size.height);

    GtkRequisition natural;
    gtk_widget_get_preferred_size(GTK_WIDGET(self->gtk_window), NULL, &natural);

    if (!size.width)  size.width  = natural.width;
    if (!size.height) size.height = natural.height;

    return size;
}

static void gtk_layer_surface_on_default_size_set(
    GtkWindow* _window,
    const GParamSpec* _pspec,
    struct layer_surface_t* layer_surface
) {
    (void)_window;
    (void)_pspec;
    layer_surface_invalidate_preferred_size(&((struct gtk_layer_surface_t*)layer_surface)->super);
}

static void gtk_layer_surface_remap(struct layer_surface_t* super) {
    if (g_list_model_get_n_items(gtk_monitors) == 0) {
        // GTK will exit if you try to map a window while there are no monitors, so don't do that
        return;
    }
    struct gtk_layer_surface_t* self = (struct gtk_layer_surface_t*)super;
    gtk_widget_unrealize(GTK_WIDGET(self->gtk_window));
    gtk_widget_map(GTK_WIDGET(self->gtk_window));
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_init_for_window(GtkWindow* window) {
    if (!GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        g_warning("Failed to initialize layer surface, not on Wayland");
        return;
    }

    if (!window) {
        g_warning("Failed to initialize layer surface, provided window is null");
        return;
    }

    if (!libwayland_shim_has_initialized()) {
        g_warning("Failed to initialize layer surface, GTK4 Layer Shell may have been linked after libwayland.");
        g_message("Move gtk4-layer-shell before libwayland-client in the linker options.");
        g_message("You may be able to fix with without recompiling by setting LD_PRELOAD=/path/to/libgtk4-layer-shell.so");
        g_message("See https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md for more info");
        return;
    }

    if (!init_and_get_layer_shell_global()) {
        g_warning("Failed to initialize layer surface, it appears your Wayland compositor doesn't support Layer Shell");
        return;
    }

    static gboolean has_installed_hook = false;
    if (!has_installed_hook) {
        has_installed_hook = true;
        layer_surface_install_hook(layer_surface_hook_callback_impl);
    }

    struct gtk_layer_surface_t* layer_surface = g_new0(struct gtk_layer_surface_t, 1);
    layer_surface->gtk_window = window;
    layer_surface->super = layer_surface_make();
    layer_surface->super.remap = gtk_layer_surface_remap;
    layer_surface->super.get_preferred_size = gtk_layer_surface_get_preferred_size;
    g_object_set_data_full(G_OBJECT(window), layer_surface_key, layer_surface, (GDestroyNotify)gtk_layer_surface_destroy);

    all_layer_surfaces = g_list_append(all_layer_surfaces, layer_surface);

    gtk_window_set_decorated(window, FALSE);
    g_signal_connect(window, "notify::default-width",  G_CALLBACK(gtk_layer_surface_on_default_size_set), layer_surface);
    g_signal_connect(window, "notify::default-height", G_CALLBACK(gtk_layer_surface_on_default_size_set), layer_surface);

    if (gtk_widget_get_mapped(GTK_WIDGET(window))) {
        layer_surface->super.remap(&layer_surface->super);
    }
}

GTK4_LAYER_SHELL_EXPORT
gboolean gtk_layer_is_layer_window(GtkWindow* window) {
    return gtk_window_get_layer_surface(window) != NULL;
}

GTK4_LAYER_SHELL_EXPORT
struct zwlr_layer_surface_v1* gtk_layer_get_zwlr_layer_surface_v1(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return NULL;
    return layer_surface->super.layer_surface;
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_set_namespace(GtkWindow* window, char const* name_space) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_set_name_space(&layer_surface->super, name_space);
}

GTK4_LAYER_SHELL_EXPORT
const char* gtk_layer_get_namespace(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    return layer_surface_get_namespace(&layer_surface->super); // NULL-safe
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_set_layer(GtkWindow* window, GtkLayerShellLayer layer) {
    g_return_if_fail(layer >= 0 && layer < GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER);
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    // Our keyboard interactivity enum matches the layer shell one
    layer_surface_set_layer(&layer_surface->super, (enum zwlr_layer_shell_v1_layer)layer);
}

GTK4_LAYER_SHELL_EXPORT
GtkLayerShellLayer gtk_layer_get_layer(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return GTK_LAYER_SHELL_LAYER_TOP;
    // Our keyboard interactivity enum matches the layer shell one
    return (GtkLayerShellLayer)layer_surface->super.layer;
}

static void gtk_layer_surface_monitor_invalidated(GdkMonitor* self, struct gtk_layer_surface_t* layer_surface) {
    if (layer_surface->monitor != self) {
        g_error("got monitor_invalidated() signal for non-current monitor, please report to GTK4 Layer Shell");
        return;
    }
    gtk_layer_set_monitor(layer_surface->gtk_window, NULL);
}

static void gtk_layer_surface_clear_monitor(struct gtk_layer_surface_t* layer_surface) {
    if (layer_surface->monitor) {
        g_signal_handlers_disconnect_by_data(layer_surface->monitor, layer_surface);
        g_object_unref(G_OBJECT(layer_surface->monitor));
    }
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_set_monitor(GtkWindow* window, GdkMonitor* monitor) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    struct wl_output* output = NULL;
    if (monitor) {
        g_return_if_fail(GDK_IS_WAYLAND_MONITOR(monitor));
        output = gdk_wayland_monitor_get_wl_output(monitor);
        g_return_if_fail(output);
    }
    gtk_layer_surface_clear_monitor(layer_surface);
    layer_surface->monitor = monitor;
    if (monitor) {
        g_object_ref(G_OBJECT(monitor));
        // Connect after hopefully allows apps to handle this first
        g_signal_connect_after(monitor, "invalidate",  G_CALLBACK(gtk_layer_surface_monitor_invalidated), layer_surface);
    }
    layer_surface_set_output(&layer_surface->super, output);
}

GTK4_LAYER_SHELL_EXPORT
GdkMonitor* gtk_layer_get_monitor(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return NULL;
    return layer_surface->monitor;
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_set_anchor(GtkWindow* window, GtkLayerShellEdge edge, gboolean anchor_to_edge) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    struct geom_edges_t anchored = layer_surface->super.anchored;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   anchored.left   = anchor_to_edge; break;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  anchored.right  = anchor_to_edge; break;
        case GTK_LAYER_SHELL_EDGE_TOP:    anchored.top    = anchor_to_edge; break;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: anchored.bottom = anchor_to_edge; break;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge);
    }
    layer_surface_set_anchor(&layer_surface->super, anchored);
}

GTK4_LAYER_SHELL_EXPORT
gboolean gtk_layer_get_anchor(GtkWindow* window, GtkLayerShellEdge edge) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return FALSE;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   return layer_surface->super.anchored.left;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  return layer_surface->super.anchored.right;
        case GTK_LAYER_SHELL_EDGE_TOP:    return layer_surface->super.anchored.top;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: return layer_surface->super.anchored.bottom;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge); return FALSE;
    }
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_set_margin(GtkWindow* window, GtkLayerShellEdge edge, int margin_size) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    struct geom_edges_t margins = layer_surface->super.margin_size;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   margins.left   = margin_size; break;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  margins.right  = margin_size; break;
        case GTK_LAYER_SHELL_EDGE_TOP:    margins.top    = margin_size; break;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: margins.bottom = margin_size; break;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge);
    }
    layer_surface_set_margin(&layer_surface->super, margins);
}

GTK4_LAYER_SHELL_EXPORT
int gtk_layer_get_margin(GtkWindow* window, GtkLayerShellEdge edge) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return 0;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   return layer_surface->super.margin_size.left;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  return layer_surface->super.margin_size.right;
        case GTK_LAYER_SHELL_EDGE_TOP:    return layer_surface->super.margin_size.top;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: return layer_surface->super.margin_size.bottom;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge); return 0;
    }
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_set_exclusive_zone(GtkWindow* window, int exclusive_zone) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_set_exclusive_zone(&layer_surface->super, exclusive_zone);
}

GTK4_LAYER_SHELL_EXPORT
int gtk_layer_get_exclusive_zone(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return 0;
    return layer_surface->super.exclusive_zone;
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_auto_exclusive_zone_enable(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_auto_exclusive_zone_enable(&layer_surface->super);
}

GTK4_LAYER_SHELL_EXPORT
gboolean gtk_layer_auto_exclusive_zone_is_enabled(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return FALSE;
    return layer_surface->super.auto_exclusive_zone;
}

GTK4_LAYER_SHELL_EXPORT
void gtk_layer_set_keyboard_mode(GtkWindow* window, GtkLayerShellKeyboardMode mode) {
    g_return_if_fail(mode >= 0 && mode < GTK_LAYER_SHELL_KEYBOARD_MODE_ENTRY_NUMBER);
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    // Our keyboard interactivity enum matches the layer shell one
    layer_surface_set_keyboard_mode(&layer_surface->super, (enum zwlr_layer_surface_v1_keyboard_interactivity)mode);
}

GTK4_LAYER_SHELL_EXPORT
GtkLayerShellKeyboardMode gtk_layer_get_keyboard_mode(GtkWindow* window) {
    struct gtk_layer_surface_t* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return GTK_LAYER_SHELL_KEYBOARD_MODE_NONE;
    // Our keyboard interactivity enum matches the layer shell one
    return (GtkLayerShellKeyboardMode)layer_surface->super.keyboard_mode;
}
