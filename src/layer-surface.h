#pragma once

#include <stdbool.h>
#include "wlr-layer-shell-unstable-v1-client.h"

struct geom_edges_t {
    int left, right, top, bottom;
};

struct geom_size_t {
    int width, height;
};

#define GEOM_SIZE_UNSET (struct geom_size_t){-1, -1}

struct layer_surface_t {
    // Virtual functions (NULL by default, can be overridden by the user of this library)

    // Ask the toolkit to unmap and remap the surface
    void (*remap)(struct layer_surface_t* super);

    // Get the preferred size of the surface from the client program. -1 (as is returned for both axes by the default
    // implementation) means to figure it out based on Wayland messages. In general this default behavior is correct,
    // however GTK seems to have a bug where if its configured with EITHER width or height as 0 it uses its preferred
    // size for both dimensions. This causes problems when anchored along one axis and not the other. By supplying an
    // implementation of this function that gets GTKs preferred widget size this issue can be bypassed.
    struct geom_size_t (*get_preferred_size)(struct layer_surface_t* super);

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
    // The last size we configured the client program with. -1 means unset, 0 means we've asked the program to decide
    // its own size. In theory we should be able to set only width or only height to 0. In practice GTK at least uses
    // its preferred size for both if either are set to 0, so we either use (0, 0) or (>0, >0).
    struct geom_size_t cached_xdg_configure_size;
    // The last size the client program set its window geometry to. -1 means unset, should never be 0 for well behaved
    // client programs.
    struct geom_size_t last_xdg_window_geom_size;
    // The last size we sent to the compositor for our layer surface's size (with the .set_size request). -1 is unset,
    // 0 means we requested the compositor give us a size, though this is only allowed for axes where we're currently
    // anchored to both edges.
    struct geom_size_t cached_layer_size_set;
    // The last size the compositor configured our layer surface with. -1 is unset. 0 means no preference from the
    // compositor. This is a hint, are allowed to ignore this size if we want.
    struct geom_size_t last_layer_configured_size;
    // If non-zero our layer surface received a configure with this serial, we passed it on to the client program's XDG
    // surface and will ack it once the client program acks its configure. Otherwise this is zero, all acks from the
    // client program can be ignored (they are for configures not originating from the compositor)
    uint32_t pending_configure_serial;
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
void layer_surface_invalidate_preferred_size(struct layer_surface_t* self); // Called when preferred size may have changed

// Each time the current process attempts to create a new xdg_surface out of a wl_surface this callback will be called.
// If the given callback returns a non-null pointer, this layer surface is used to override the XDG surface. Else the
// XDG surface is created normally. Thus must be used for any layer surfaces to be displayed.
typedef struct layer_surface_t* (*layer_surface_hook_callback_t)(struct wl_surface*);
void layer_surface_install_hook(layer_surface_hook_callback_t callback);
