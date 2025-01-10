#pragma once

#include "wlr-layer-shell-unstable-v1-client.h"
#include "gtk4-layer-shell.h"
#include <gtk/gtk.h>

struct wl_surface;
struct xdg_surface;
struct xdg_positioner;

struct geom_edges_t {
    int left, right, top, bottom;
};

struct geom_size_t {
    int width, height;
};

// Functions that mutate this structure should all be in layer-surface.c to make the logic easier to understand
// Struct is declared in this header to prevent the need for excess getters
struct layer_surface_t {
    GtkWindow* gtk_window;

    void (*remap)(struct layer_surface_t* super);

    // Can be set at any time
    struct geom_edges_t anchored; // Logically booleans, the edges of the output this layer surface is currently anchored to
    struct geom_edges_t margin_size; // The gap between each edge of the output and this layer surface (only applicable for anchored edges)
    int exclusive_zone; // The current exclusive zone(set either explicitly or automatically)
    bool auto_exclusive_zone; // If to automatically change the exclusive zone to match the window size
    enum zwlr_layer_surface_v1_keyboard_interactivity keyboard_mode; // Type of keyboard interactivity enabled for this surface
    enum zwlr_layer_shell_v1_layer layer; // The current layer, needs surface recreation on old layer shell versions

    // Need the surface to be recreated to change
    struct wl_output* output; // Can be null
    const char* name_space; // Can be null, freed on destruction

    // Not set by user requests
    struct zwlr_layer_surface_v1* layer_surface; // The actual layer surface Wayland object (can be NULL)
    struct geom_size_t preferred_size; // Should always be non-zero
    struct geom_size_t cached_xdg_configure_size; // The last size we configured GTK's XDG surface with
    struct geom_size_t cached_layer_size_set; // The last size we set the layer surface to with the compositor
    struct geom_size_t last_layer_configured_size; // The last size our layer surface received from the compositor
    uint32_t pending_configure_serial; // If non-zero our layer surface received a configure with this serial, we passed
      // it on to GTK's XDG surface and will ack it once GTK acks it's configure. Otherwise this is zero, all acks from
      // GTK can be ignored(they are for configures not originating from the compositor)
    struct xdg_surface* client_facing_xdg_surface;
    struct xdg_toplevel* client_facing_xdg_toplevel;
    bool has_initial_layer_shell_configure;
};

struct layer_surface_t layer_surface_make();
void layer_surface_uninit(struct layer_surface_t* self);

// Surface is remapped in order to set
void layer_surface_set_output(struct layer_surface_t* self, struct wl_output* output); // Can be null for default
void layer_surface_set_name_space(struct layer_surface_t* self, char const* name_space); // Makes a copy of the string, can be null

// Returns the effective namespace(default if unset). Does not return ownership. Never returns NULL. Handles NULL self.
const char* layer_surface_get_namespace(struct layer_surface_t* self);

// Can be set without remapping the surface
void layer_surface_set_layer(struct layer_surface_t* self, enum zwlr_layer_shell_v1_layer layer); // Remaps surface on old layer shell versions
void layer_surface_set_anchor(struct layer_surface_t* self, struct geom_edges_t anchors); // anchor values are treated as booleans
void layer_surface_set_margin(struct layer_surface_t* self, struct geom_edges_t margins);
void layer_surface_set_exclusive_zone(struct layer_surface_t* self, int exclusive_zone);
void layer_surface_auto_exclusive_zone_enable(struct layer_surface_t* self);
void layer_surface_set_keyboard_mode(
    struct layer_surface_t* self,
    enum zwlr_layer_surface_v1_keyboard_interactivity mode
);
void layer_surface_set_preferred_size(struct layer_surface_t* self, struct geom_size_t size); // -1 to unset

extern struct layer_surface_t* (*get_layer_surface_for_wl_surface)(struct wl_surface* wl_surface);
