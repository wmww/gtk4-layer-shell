#include "wayland-utils.h"
#include "layer-surface.h"
#include "libwayland-shim.h"

#include <gdk/wayland/gdkwayland.h>

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

static LayerSurface* gtk_window_get_layer_surface_or_warn(GtkWindow* window) {
    g_return_val_if_fail(window, NULL);
    LayerSurface* layer_surface = gtk_window_get_layer_surface(window);
    if (!layer_surface) {
        g_warning("GtkWindow is not a layer surface. Make sure you called gtk_layer_init_for_window()");
        return NULL;
    }
    return layer_surface;
}

void gtk_layer_init_for_window(GtkWindow* window) {
    gtk_wayland_init_if_needed();
    layer_surface_new(window);
}

gboolean gtk_layer_is_layer_window(GtkWindow* window) {
    return gtk_window_get_layer_surface(window) != NULL;
}

struct zwlr_layer_surface_v1* gtk_layer_get_zwlr_layer_surface_v1(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return NULL;
    return layer_surface->layer_surface;
}

void gtk_layer_set_namespace(GtkWindow* window, char const* name_space) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_set_name_space(layer_surface, name_space);
}

const char* gtk_layer_get_namespace(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    return layer_surface_get_namespace(layer_surface); // NULL-safe
}

void gtk_layer_set_layer(GtkWindow* window, GtkLayerShellLayer layer) {
    g_return_if_fail(layer >= 0 && layer < GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER);
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    // Our keyboard interactivity enum matches the layer shell one
    layer_surface_set_layer(layer_surface, (enum zwlr_layer_shell_v1_layer)layer);
}

GtkLayerShellLayer gtk_layer_get_layer(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return GTK_LAYER_SHELL_LAYER_TOP;
    // Our keyboard interactivity enum matches the layer shell one
    return (GtkLayerShellLayer)layer_surface->layer;
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
    layer_surface_set_output(layer_surface, output);
}

GdkMonitor * gtk_layer_get_monitor(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return NULL;
    // TODO
    return NULL;
}

void gtk_layer_set_anchor(GtkWindow* window, GtkLayerShellEdge edge, gboolean anchor_to_edge) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    struct geom_edges_t anchored = layer_surface->anchored;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   anchored.left   = anchor_to_edge; break;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  anchored.right  = anchor_to_edge; break;
        case GTK_LAYER_SHELL_EDGE_TOP:    anchored.top    = anchor_to_edge; break;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: anchored.bottom = anchor_to_edge; break;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge);
    }
    layer_surface_set_anchor(layer_surface, anchored);
}

gboolean gtk_layer_get_anchor(GtkWindow* window, GtkLayerShellEdge edge) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return FALSE;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   return layer_surface->anchored.left;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  return layer_surface->anchored.right;
        case GTK_LAYER_SHELL_EDGE_TOP:    return layer_surface->anchored.top;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: return layer_surface->anchored.bottom;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge); return FALSE;
    }
}

void gtk_layer_set_margin(GtkWindow* window, GtkLayerShellEdge edge, int margin_size) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    struct geom_edges_t margins = layer_surface->margin_size;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   margins.left   = margin_size; break;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  margins.right  = margin_size; break;
        case GTK_LAYER_SHELL_EDGE_TOP:    margins.top    = margin_size; break;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: margins.bottom = margin_size; break;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge);
    }
    layer_surface_set_margin(layer_surface, margins);
}

int gtk_layer_get_margin(GtkWindow* window, GtkLayerShellEdge edge) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return 0;
    switch (edge) {
        case GTK_LAYER_SHELL_EDGE_LEFT:   return layer_surface->margin_size.left;
        case GTK_LAYER_SHELL_EDGE_RIGHT:  return layer_surface->margin_size.right;
        case GTK_LAYER_SHELL_EDGE_TOP:    return layer_surface->margin_size.top;
        case GTK_LAYER_SHELL_EDGE_BOTTOM: return layer_surface->margin_size.bottom;
        default: g_warning("Invalid GtkLayerShellEdge %d", edge); return 0;
    }
}

void gtk_layer_set_exclusive_zone(GtkWindow* window, int exclusive_zone) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_set_exclusive_zone(layer_surface, exclusive_zone);
}

int gtk_layer_get_exclusive_zone(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return 0;
    return layer_surface->exclusive_zone;
}

void gtk_layer_auto_exclusive_zone_enable(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    layer_surface_auto_exclusive_zone_enable(layer_surface);
}

gboolean gtk_layer_auto_exclusive_zone_is_enabled(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return FALSE;
    return layer_surface->auto_exclusive_zone;
}

void gtk_layer_set_keyboard_mode(GtkWindow* window, GtkLayerShellKeyboardMode mode) {
    g_return_if_fail(mode >= 0 && mode < GTK_LAYER_SHELL_KEYBOARD_MODE_ENTRY_NUMBER);
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return;
    // Our keyboard interactivity enum matches the layer shell one
    layer_surface_set_keyboard_mode(layer_surface, (enum zwlr_layer_surface_v1_keyboard_interactivity)mode);
}

GtkLayerShellKeyboardMode gtk_layer_get_keyboard_mode(GtkWindow* window) {
    LayerSurface* layer_surface = gtk_window_get_layer_surface_or_warn(window);
    if (!layer_surface) return GTK_LAYER_SHELL_KEYBOARD_MODE_NONE;
    // Our keyboard interactivity enum matches the layer shell one
    return (GtkLayerShellKeyboardMode)layer_surface->keyboard_mode;
}
