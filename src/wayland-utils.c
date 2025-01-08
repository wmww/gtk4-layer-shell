#include "wayland-utils.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <gdk/wayland/gdkwayland.h>

static struct wl_registry *wl_registry_global = NULL;
static struct zwlr_layer_shell_v1 *layer_shell_global = NULL;
static struct ext_session_lock_manager_v1 *session_lock_manager_global = NULL;

static gboolean has_initialized = FALSE;

struct zwlr_layer_shell_v1 *
gtk_wayland_get_layer_shell_global ()
{
    return layer_shell_global;
}

struct ext_session_lock_manager_v1 *
gtk_wayland_get_session_lock_manager_global ()
{
    return session_lock_manager_global;
}

static void
wl_registry_handle_global (void *_data,
                           struct wl_registry *registry,
                           uint32_t id,
                           const char *interface,
                           uint32_t version)
{
    (void)_data;

    // pull out needed globals
    if (strcmp (interface, zwlr_layer_shell_v1_interface.name) == 0) {
        g_warn_if_fail (zwlr_layer_shell_v1_interface.version >= 3);
        layer_shell_global = wl_registry_bind (registry,
                                               id,
                                               &zwlr_layer_shell_v1_interface,
                                               MIN((uint32_t)zwlr_layer_shell_v1_interface.version, version));
    }

    if (strcmp (interface, ext_session_lock_manager_v1_interface.name) == 0) {
        g_warn_if_fail (ext_session_lock_manager_v1_interface.version >= 1);
        session_lock_manager_global = wl_registry_bind (registry,
                                                        id,
                                                        &ext_session_lock_manager_v1_interface,
                                                        MIN((uint32_t)ext_session_lock_manager_v1_interface.version, version));
    }
}

static void
wl_registry_handle_global_remove (void *_data,
                                  struct wl_registry *_registry,
                                  uint32_t _id)
{
    (void)_data;
    (void)_registry;
    (void)_id;
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_handle_global,
    .global_remove = wl_registry_handle_global_remove,
};

void
gtk_wayland_init_if_needed ()
{
    if (has_initialized)
        return;

    gtk_init ();
    GdkDisplay *gdk_display = gdk_display_get_default ();
    g_return_if_fail (gdk_display);
    g_return_if_fail (GDK_IS_WAYLAND_DISPLAY (gdk_display));

    struct wl_display *wl_display = gdk_wayland_display_get_wl_display (gdk_display);
    wl_registry_global = wl_display_get_registry (wl_display);
    wl_registry_add_listener (wl_registry_global, &wl_registry_listener, NULL);
    wl_display_roundtrip (wl_display);

    if (!layer_shell_global)
        g_warning ("it appears your Wayland compositor does not support the Layer Shell protocol");

    has_initialized = TRUE;
}

enum zwlr_layer_shell_v1_layer
gtk_layer_shell_layer_get_zwlr_layer_shell_v1_layer (GtkLayerShellLayer layer)
{
    switch (layer)
    {
    case GTK_LAYER_SHELL_LAYER_BACKGROUND: return ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
    case GTK_LAYER_SHELL_LAYER_BOTTOM: return ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
    case GTK_LAYER_SHELL_LAYER_TOP: return ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    case GTK_LAYER_SHELL_LAYER_OVERLAY: return ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    default:
        g_critical ("Invalid GtkLayerShellLayer %d", layer);
        return ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
    }
}

uint32_t
gtk_layer_shell_edge_array_get_zwlr_layer_shell_v1_anchor (gboolean edges[GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER])
{
    uint32_t anchor = 0;
    if (edges[GTK_LAYER_SHELL_EDGE_LEFT]) anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
    if (edges[GTK_LAYER_SHELL_EDGE_RIGHT]) anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    if (edges[GTK_LAYER_SHELL_EDGE_TOP]) anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
    if (edges[GTK_LAYER_SHELL_EDGE_BOTTOM]) anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    return anchor;
}

enum xdg_positioner_gravity
gdk_gravity_get_xdg_positioner_gravity (GdkGravity gravity)
{
    switch (gravity)
    {
    case GDK_GRAVITY_NORTH_WEST: return XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT;
    case GDK_GRAVITY_NORTH: return XDG_POSITIONER_GRAVITY_BOTTOM;
    case GDK_GRAVITY_NORTH_EAST: return XDG_POSITIONER_GRAVITY_BOTTOM_LEFT;
    case GDK_GRAVITY_WEST: return XDG_POSITIONER_GRAVITY_RIGHT;
    case GDK_GRAVITY_CENTER: return XDG_POSITIONER_GRAVITY_NONE;
    case GDK_GRAVITY_EAST: return XDG_POSITIONER_GRAVITY_LEFT;
    case GDK_GRAVITY_SOUTH_WEST: return XDG_POSITIONER_GRAVITY_TOP_RIGHT;
    case GDK_GRAVITY_SOUTH: return XDG_POSITIONER_GRAVITY_TOP;
    case GDK_GRAVITY_SOUTH_EAST: return XDG_POSITIONER_GRAVITY_TOP_LEFT;
    case GDK_GRAVITY_STATIC: return XDG_POSITIONER_GRAVITY_NONE;
    default:
        g_critical ("Invalid GdkGravity %d", gravity);
        return XDG_POSITIONER_GRAVITY_NONE;
    }
}

enum xdg_positioner_anchor
gdk_gravity_get_xdg_positioner_anchor (GdkGravity anchor)
{
    switch (anchor)
    {
    case GDK_GRAVITY_NORTH_WEST: return XDG_POSITIONER_ANCHOR_TOP_LEFT;
    case GDK_GRAVITY_NORTH: return XDG_POSITIONER_ANCHOR_TOP;
    case GDK_GRAVITY_NORTH_EAST: return XDG_POSITIONER_ANCHOR_TOP_RIGHT;
    case GDK_GRAVITY_WEST: return XDG_POSITIONER_ANCHOR_LEFT;
    case GDK_GRAVITY_CENTER: return XDG_POSITIONER_ANCHOR_NONE;
    case GDK_GRAVITY_EAST: return XDG_POSITIONER_ANCHOR_RIGHT;
    case GDK_GRAVITY_SOUTH_WEST: return XDG_POSITIONER_ANCHOR_BOTTOM_LEFT;
    case GDK_GRAVITY_SOUTH: return XDG_POSITIONER_ANCHOR_BOTTOM;
    case GDK_GRAVITY_SOUTH_EAST: return XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT;
    case GDK_GRAVITY_STATIC: return XDG_POSITIONER_ANCHOR_NONE;
    default:
        g_critical ("Invalid GdkGravity %d", anchor);
        return XDG_POSITIONER_ANCHOR_NONE;
    }
}

enum xdg_positioner_constraint_adjustment
gdk_anchor_hints_get_xdg_positioner_constraint_adjustment (GdkAnchorHints hints)
{
    enum xdg_positioner_constraint_adjustment adjustment = 0;
    if (hints & GDK_ANCHOR_FLIP_X) adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X;
    if (hints & GDK_ANCHOR_FLIP_Y) adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y;
    if (hints & GDK_ANCHOR_SLIDE_X) adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X;
    if (hints & GDK_ANCHOR_SLIDE_Y) adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y;
    if (hints & GDK_ANCHOR_RESIZE_X) adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_X;
    if (hints & GDK_ANCHOR_RESIZE_Y) adjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_Y;
    return adjustment;
}
