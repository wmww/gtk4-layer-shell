#ifndef SESSSION_LOCK_H
#define SESSSION_LOCK_H

#include "ext-session-lock-v1-client.h"
#include "gtk4-layer-shell.h"
#include <gtk/gtk.h>

void session_lock_lock (GtkLayerShellSessionLock *self);

void session_lock_destroy (GtkLayerShellSessionLock *self);

void session_lock_unlock_and_destroy (GtkLayerShellSessionLock *self);

void session_lock_create_surface_for_window (GtkLayerShellSessionLock *self, GtkWindow *gtk_window, GdkMonitor *gdk_monitor);

typedef struct _SessionLockSurface SessionLockSurface;

struct _SessionLockSurface
{
    GtkWindow *gtk_window;
    GdkMonitor *monitor;

    // Not set by user requests
    struct ext_session_lock_v1 *lock;
    struct ext_session_lock_surface_v1 *lock_surface; // Can be null
    struct xdg_surface *client_facing_xdg_surface;
    struct xdg_toplevel *client_facing_xdg_toplevel;

    bool can_commit;
};

// Used by libwayland wrappers
gboolean session_lock_surface_handle_request (
    struct wl_proxy *proxy,
    uint32_t opcode,
    const struct wl_interface *interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument *args,
    struct wl_proxy **ret_proxy);

#endif // SESSSION_LOCK_H
