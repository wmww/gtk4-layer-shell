#include "wayland-utils.h"
#include "layer-surface.h"
#include "libwayland-shim.h"

#include <gdk/wayland/gdkwayland.h>

static const char* layer_surface_key = "wayland_layer_surface";
static GList* all_layer_surfaces = NULL;

typedef struct {
    struct layer_surface_t super;
    GdkMonitor* monitor;
} LayerSurface;

guint gtk_layer_get_major_version() {
    return GTK_LAYER_SHELL_MAJOR;
}

guint gtk_layer_get_minor_version() {
    return GTK_LAYER_SHELL_MINOR;
}

guint gtk_layer_get_micro_version() {
    return GTK_LAYER_SHELL_MICRO;
}

gboolean gtk_layer_is_supported() {
    gtk_wayland_init_if_needed();
    return libwayland_shim_has_initialized() && gtk_wayland_get_layer_shell_global() != NULL;
}

guint gtk_layer_get_protocol_version() {
    if (!GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        return 0;
    }
    gtk_wayland_init_if_needed();
    struct zwlr_layer_shell_v1* layer_shell_global = gtk_wayland_get_layer_shell_global();
    if (!layer_shell_global) {
        return 0;
    }
    return zwlr_layer_shell_v1_get_version(layer_shell_global);
}

static LayerSurface* gtk_window_get_layer_surface(GtkWindow* gtk_window) {
    if (!gtk_window) return NULL;
    return g_object_get_data(G_OBJECT(gtk_window), layer_surface_key);
}

static LayerSurface* gtk_window_get_layer_surface_or_warn(GtkWindow* window) {
    g_return_val_if_fail(window, NULL);
    LayerSurface* layer_surface = gtk_window_get_layer_surface(window);
    if (!layer_surface) {
        g_warning("GtkWindow is not a layer surface. Make sure you called gtk_layer_init_for_window()");
        return NULL;
    }
    return layer_surface;
}

static void layer_surface_destroy(LayerSurface* self) {
    layer_surface_uninit(&self->super);
    all_layer_surfaces = g_list_remove(all_layer_surfaces, self);
    g_free(self);
}

static gint find_layer_surface_with_wl_surface(gconstpointer layer_surface, gconstpointer needle) {
    struct layer_surface_t const* self = layer_surface;
    GdkSurface* gdk_surface = gtk_native_get_surface(GTK_NATIVE(self->gtk_window));
    if (!gdk_surface) return 1;
    struct wl_surface* wl_surface = gdk_wayland_surface_get_wl_surface(gdk_surface);
    return wl_surface == needle ? 0 : 1;
}

static struct layer_surface_t* get_layer_surface_for_wl_surface_impl(struct wl_surface* wl_surface) {
    GList* layer_surface_entry = g_list_find_custom(
        all_layer_surfaces,
        wl_surface,
        find_layer_surface_with_wl_surface
    );
    return layer_surface_entry ? layer_surface_entry->data : NULL;
}

static void layer_surface_update_preferred_size(LayerSurface* self) {
    struct geom_size_t size = {0};
    gtk_window_get_default_size(self->super.gtk_window, &size.width, &size.height);

    GtkRequisition natural;
    gtk_widget_get_preferred_size(GTK_WIDGET(self->super.gtk_window), NULL, &natural);

    if (!size.width)  size.width  = natural.width;
    if (!size.height) size.height = natural.height;
    layer_surface_set_preferred_size(&self->super, size);
}

static void layer_surface_on_default_size_set(
    GtkWindow* _window,
    const GParamSpec* _pspec,
    struct layer_surface_t* layer_surface
) {
    (void)_window;
    (void)_pspec;
    layer_surface_update_preferred_size((LayerSurface*)layer_surface);
}

static void layer_surface_remap_impl(struct layer_surface_t* self) {
    gtk_widget_unrealize(GTK_WIDGET(self->gtk_window));
    gtk_widget_map(GTK_WIDGET(self->gtk_window));
}

void gtk_layer_init_for_window(GtkWindow* window) {
    gtk_wayland_init_if_needed();
    get_layer_surface_for_wl_surface = get_layer_surface_for_wl_surface_impl;

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

    if (!gtk_wayland_get_layer_shell_global()) {
        g_warning("Failed to initialize layer surface, it appears your Wayland compositor doesn't support Layer Shell");
        return;
    }

    LayerSurface* layer_surface = g_new0(LayerSurface, 1);
    layer_surface->super = layer_surface_make();
    layer_surface->super.remap = layer_surface_remap_impl;
    layer_surface->super.gtk_window = window;
    layer_surface_update_preferred_size(layer_surface);
    g_object_set_data_full(G_OBJECT(window), layer_surface_key, layer_surface, (GDestroyNotify)layer_surface_destroy);

    all_layer_surfaces = g_list_append(all_layer_surfaces, layer_surface);

    gtk_window_set_decorated(window, FALSE);
    g_signal_connect(window, "notify::default-width", G_CALLBACK(layer_surface_on_default_size_set), layer_surface);
    g_signal_connect(window, "notify::default-height", G_CALLBACK(layer_surface_on_default_size_set), layer_surface);

    if (gtk_widget_get_mapped(GTK_WIDGET(window))) {
        layer_surface->super.remap(&layer_surface->super);
    }
}

gboolean gtk_layer_is_layer_window(GtkWindow* window) {
    return gtk_window_get_layer_surface(window) != NULL;
}

struct zwlr_layer_surface_v1* gtk_layer_get_zwlr_layer_surface_v1(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return NULL;
    return layer_surface->super.layer_surface;
}

void gtk_layer_set_namespace(GtkWindow* window, char const* name_space) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_set_name_space(&layer_surface->super, name_space);
}

const char* gtk_layer_get_namespace(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    return layer_surface_get_namespace(&layer_surface->super); // NULL-safe
}

void gtk_layer_set_layer(GtkWindow* window, GtkLayerShellLayer layer) {
    g_return_if_fail(layer >= 0 && layer < GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER);
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    // Our keyboard interactivity enum matches the layer shell one
    layer_surface_set_layer(&layer_surface->super, (enum zwlr_layer_shell_v1_layer)layer);
}

GtkLayerShellLayer gtk_layer_get_layer(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return GTK_LAYER_SHELL_LAYER_TOP;
    // Our keyboard interactivity enum matches the layer shell one
    return (GtkLayerShellLayer)layer_surface->super.layer;
}

void gtk_layer_set_monitor(GtkWindow* window, GdkMonitor* monitor) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    struct wl_output* output = NULL;
    if (monitor) {
        g_return_if_fail(GDK_IS_WAYLAND_MONITOR(monitor));
        output = gdk_wayland_monitor_get_wl_output(monitor);
        g_return_if_fail(output);
    }
    layer_surface_set_output(&layer_surface->super, output);
    layer_surface->monitor = monitor;
}

GdkMonitor* gtk_layer_get_monitor(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return NULL;
    return layer_surface->monitor;
}

void gtk_layer_set_anchor(GtkWindow* window, GtkLayerShellEdge edge, gboolean anchor_to_edge) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
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

gboolean gtk_layer_get_anchor(GtkWindow* window, GtkLayerShellEdge edge) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return FALSE;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   return layer_surface->super.anchored.left;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  return layer_surface->super.anchored.right;
        case GTK_LAYER_SHELL_EDGE_TOP:    return layer_surface->super.anchored.top;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: return layer_surface->super.anchored.bottom;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge); return FALSE;
    }
}

void gtk_layer_set_margin(GtkWindow* window, GtkLayerShellEdge edge, int margin_size) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
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

int gtk_layer_get_margin(GtkWindow* window, GtkLayerShellEdge edge) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return 0;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   return layer_surface->super.margin_size.left;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  return layer_surface->super.margin_size.right;
        case GTK_LAYER_SHELL_EDGE_TOP:    return layer_surface->super.margin_size.top;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: return layer_surface->super.margin_size.bottom;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge); return 0;
    }
}

void gtk_layer_set_exclusive_zone(GtkWindow* window, int exclusive_zone) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_set_exclusive_zone(&layer_surface->super, exclusive_zone);
}

int gtk_layer_get_exclusive_zone(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return 0;
    return layer_surface->super.exclusive_zone;
}

void gtk_layer_auto_exclusive_zone_enable(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_auto_exclusive_zone_enable(&layer_surface->super);
}

gboolean gtk_layer_auto_exclusive_zone_is_enabled(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return FALSE;
    return layer_surface->super.auto_exclusive_zone;
}

void gtk_layer_set_keyboard_mode(GtkWindow* window, GtkLayerShellKeyboardMode mode) {
    g_return_if_fail(mode >= 0 && mode < GTK_LAYER_SHELL_KEYBOARD_MODE_ENTRY_NUMBER);
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    // Our keyboard interactivity enum matches the layer shell one
    layer_surface_set_keyboard_mode(&layer_surface->super, (enum zwlr_layer_surface_v1_keyboard_interactivity)mode);
}

GtkLayerShellKeyboardMode gtk_layer_get_keyboard_mode(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return GTK_LAYER_SHELL_KEYBOARD_MODE_NONE;
    // Our keyboard interactivity enum matches the layer shell one
    return (GtkLayerShellKeyboardMode)layer_surface->super.keyboard_mode;
}
