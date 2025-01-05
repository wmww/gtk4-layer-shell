#include "session-lock.h"

#include "wayland-utils.h"
#include "libwayland-shim.h"

#include "ext-session-lock-v1-client.h"
#include "xdg-shell-client.h"

#include <gtk/gtk.h>
#include <gdk/wayland/gdkwayland.h>

static const char *lock_surface_key = "wayland_lock_surface";
GList* all_lock_surfaces = NULL;

struct _GtkLayerShellSessionLock
{
    GObject parent_instance;
};

typedef struct {
    struct ext_session_lock_v1 *lock;
} GtkLayerShellSessionLockPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GtkLayerShellSessionLock, gtk_layer_session_lock, G_TYPE_OBJECT)

static void
gtk_layer_session_lock_init (GtkLayerShellSessionLock *self)
{
    if (!GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ())) {
        g_warning ("Failed to initialize session lock, not on Wayland");
        return;
    }

    if (!libwayland_shim_has_initialized ()) {
        g_warning ("Failed to initialize session lock, GTK4 Layer Shell may have been linked after libwayland.");
        g_message ("Move gtk4-layer-shell before libwayland-client in the linker options.");
        g_message ("You may be able to fix with without recompiling by setting LD_PRELOAD=/path/to/libgtk4-layer-shell.so");
        g_message ("See https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md for more info");
        return;
    }

    if (!gtk_wayland_get_session_lock_manager_global ()) {
        g_warning ("Failed to initialize session lock, it appears your Wayland compositor doesn't support the session lock protocol");
        return;
    }

    GtkLayerShellSessionLockPrivate *priv = gtk_layer_session_lock_get_instance_private (self);
    g_return_if_fail (priv);
}

enum {
    SESSION_LOCK_SIGNAL_LOCKED = 1,
    SESSION_LOCK_SIGNAL_FINISHED,
    SESSION_LOCK_SIGNAL_LAST
};

static guint session_lock_signals[SESSION_LOCK_SIGNAL_LAST] = { 0, };

static void
gtk_layer_session_lock_class_init (GtkLayerShellSessionLockClass *cclass)
{
    session_lock_signals[SESSION_LOCK_SIGNAL_LOCKED] = g_signal_new (
        "locked",
        G_TYPE_FROM_CLASS (cclass),
        G_SIGNAL_RUN_FIRST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        0
    );

    session_lock_signals[SESSION_LOCK_SIGNAL_FINISHED] = g_signal_new (
        "finished",
        G_TYPE_FROM_CLASS (cclass),
        G_SIGNAL_RUN_FIRST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        0
    );
}

static void
session_lock_handle_locked (void *data,
                            struct ext_session_lock_v1 *manager)
{
    (void)manager;
    GtkLayerShellSessionLock *self = (GtkLayerShellSessionLock *)data;
    g_signal_emit (self, session_lock_signals[SESSION_LOCK_SIGNAL_LOCKED], 0);
}

static void
session_lock_handle_finished (void *data,
                              struct ext_session_lock_v1 *manager)
{
    (void)manager;
    GtkLayerShellSessionLock *self = (GtkLayerShellSessionLock *)data;
    g_signal_emit (self, session_lock_signals[SESSION_LOCK_SIGNAL_FINISHED], 0);
}

static void
session_lock_surface_unmap (SessionLockSurface *self)
{
    if (self->lock_surface) {
        ext_session_lock_surface_v1_destroy (self->lock_surface);
        self->lock_surface = NULL;
    }

    libwayland_shim_clear_client_proxy_data((struct wl_proxy *)self->client_facing_xdg_surface);
    libwayland_shim_clear_client_proxy_data((struct wl_proxy *)self->client_facing_xdg_toplevel);

    self->client_facing_xdg_surface = NULL;
    self->client_facing_xdg_toplevel = NULL;
    self->can_commit = false;
}

static void
session_lock_surface_destroy (SessionLockSurface *self)
{
    session_lock_surface_unmap (self);
    all_lock_surfaces = g_list_remove (all_lock_surfaces, self);
    g_free (self);
}

static void
session_lock_surface_handle_configure (void *data,
                                       struct ext_session_lock_surface_v1 *surface,
                                       uint32_t serial,
                                       uint32_t width,
                                       uint32_t height)
{
    (void)surface;
    SessionLockSurface *self = data;

    self->can_commit = true;

    ext_session_lock_surface_v1_ack_configure (self->lock_surface, serial);

    // trigger a commit

    struct wl_array states;
    wl_array_init(&states);
    {
        uint32_t *state = wl_array_add(&states, sizeof(uint32_t));
        g_assert(state);
        *state = XDG_TOPLEVEL_STATE_ACTIVATED;
    }
    {
        uint32_t *state = wl_array_add(&states, sizeof(uint32_t));
        g_assert(state);
        *state = XDG_TOPLEVEL_STATE_MAXIMIZED;
    }

    LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
        xdg_toplevel_listener,
        self->client_facing_xdg_toplevel,
        configure,
        self->client_facing_xdg_toplevel,
        width, height,
        &states);
    wl_array_release(&states);

    LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
        xdg_surface_listener,
        self->client_facing_xdg_surface,
        configure,
        self->client_facing_xdg_surface,
        serial);
}

static const struct ext_session_lock_surface_v1_listener session_lock_surface_listener = {
    .configure = session_lock_surface_handle_configure,
};

static const struct ext_session_lock_v1_listener session_lock_listener = {
    .locked = session_lock_handle_locked,
    .finished = session_lock_handle_finished,
};

void
session_lock_lock (GtkLayerShellSessionLock *self)
{
    GtkLayerShellSessionLockPrivate *priv = gtk_layer_session_lock_get_instance_private (self);
    g_return_if_fail (priv);

    struct ext_session_lock_manager_v1 *manager = gtk_wayland_get_session_lock_manager_global ();
    g_return_if_fail (manager);

    priv->lock = ext_session_lock_manager_v1_lock (manager);
    ext_session_lock_v1_add_listener (priv->lock, &session_lock_listener, self);
}

void
session_lock_destroy (GtkLayerShellSessionLock *self)
{
    GtkLayerShellSessionLockPrivate *priv = gtk_layer_session_lock_get_instance_private (self);
    g_return_if_fail (priv);
    g_return_if_fail (priv->lock);

    ext_session_lock_v1_destroy (priv->lock);
    priv->lock = NULL;
}

void
session_lock_unlock_and_destroy (GtkLayerShellSessionLock *self)
{
    GtkLayerShellSessionLockPrivate *priv = gtk_layer_session_lock_get_instance_private (self);
    g_return_if_fail (priv);
    g_return_if_fail (priv->lock);

    ext_session_lock_v1_unlock_and_destroy (priv->lock);
    priv->lock = NULL;
}

static void
session_lock_window_realize (GtkWindow *gtk_window, void *data)
{
    (void)data;
    SessionLockSurface *surface = g_object_get_data (G_OBJECT (gtk_window), lock_surface_key);

    GdkSurface *gdk_surface = gtk_native_get_surface (GTK_NATIVE (gtk_window));
    struct wl_surface *wl_surface = gdk_wayland_surface_get_wl_surface (gdk_surface);
    struct wl_output *wl_output = gdk_wayland_monitor_get_wl_output (surface->monitor);

    surface->lock_surface = ext_session_lock_v1_get_lock_surface (surface->lock, wl_surface, wl_output);
    g_return_if_fail (surface->lock_surface);

    ext_session_lock_surface_v1_add_listener (surface->lock_surface, &session_lock_surface_listener, surface);
}

void
session_lock_create_surface_for_window (GtkLayerShellSessionLock *self, GtkWindow *gtk_window, GdkMonitor *gdk_monitor)
{
    GtkLayerShellSessionLockPrivate *priv = gtk_layer_session_lock_get_instance_private (self);
    g_return_if_fail (priv);
    g_return_if_fail (priv->lock);

    SessionLockSurface *surface = g_new0 (SessionLockSurface, 1);

    surface->gtk_window = gtk_window;
    surface->monitor = gdk_monitor;
    surface->lock = priv->lock;
    surface->can_commit = false;

    all_lock_surfaces = g_list_append (all_lock_surfaces, surface);

    g_object_set_data_full (G_OBJECT (gtk_window),
                            lock_surface_key,
                            surface,
                            (GDestroyNotify) session_lock_surface_destroy);

    gtk_window_set_decorated (gtk_window, FALSE);

    g_signal_connect(gtk_window, "realize", G_CALLBACK (session_lock_window_realize), NULL);
}

static void
stubbed_xdg_toplevel_handle_destroy (void* data, struct wl_proxy *proxy)
{
    (void)proxy;
    SessionLockSurface *self = (SessionLockSurface *)data;
    session_lock_surface_unmap(self);
}

static struct wl_proxy *
stubbed_xdg_surface_handle_request (
    void* data,
    struct wl_proxy *proxy,
    uint32_t opcode,
    const struct wl_interface *interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument *args)
{
    (void)interface; (void)flags; (void)args;
    SessionLockSurface *self = (SessionLockSurface *)data;

    if (opcode == XDG_SURFACE_GET_TOPLEVEL) {
        struct wl_proxy *toplevel = libwayland_shim_create_client_proxy (
            proxy,
            &xdg_toplevel_interface,
            version,
            NULL,
            stubbed_xdg_toplevel_handle_destroy,
            data);
        self->client_facing_xdg_toplevel = (struct xdg_toplevel *)toplevel;
        return toplevel;
    } else {
        return NULL;
    }
}

static void
stubbed_xdg_surface_handle_destroy (void* data, struct wl_proxy *proxy)
{
    (void)proxy;
    SessionLockSurface *self = (SessionLockSurface *)data;
    session_lock_surface_unmap(self);
}

gint find_lock_surface_by_id (gconstpointer lock_surface, gconstpointer needle_ptr)
{
    const uint32_t needle = *(uint32_t *)needle_ptr;
    const SessionLockSurface *self = lock_surface;
    GdkSurface *gdk_surface = gtk_native_get_surface (GTK_NATIVE (self->gtk_window));
    if (!gdk_surface) return 1;

    struct wl_surface *wl_surface = gdk_wayland_surface_get_wl_surface (gdk_surface);
    uint32_t id = wl_proxy_get_id((struct wl_proxy *)wl_surface);

    return id == needle ? 0 : 1;
}

gboolean
session_lock_surface_handle_request (
    struct wl_proxy *proxy,
    uint32_t opcode,
    const struct wl_interface *interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument *args,
    struct wl_proxy **ret_proxy)
{
    (void)interface;
    (void)flags;
    const char* type = proxy->object.interface->name;
    if (strcmp(type, xdg_wm_base_interface.name) == 0) {
        if (opcode == XDG_WM_BASE_GET_XDG_SURFACE) {
            struct wl_surface *wl_surface = (struct wl_surface *)args[1].o;
            uint32_t id = wl_proxy_get_id((struct wl_proxy *)wl_surface);

            GList *lock_surface_entry = g_list_find_custom (all_lock_surfaces, &id, find_lock_surface_by_id);
            if (lock_surface_entry) {
                SessionLockSurface *self = lock_surface_entry->data;
                struct wl_proxy *xdg_surface = libwayland_shim_create_client_proxy (
                    proxy,
                    &xdg_surface_interface,
                    version,
                    stubbed_xdg_surface_handle_request,
                    stubbed_xdg_surface_handle_destroy,
                    self);
                self->client_facing_xdg_surface = (struct xdg_surface *)xdg_surface;
                *ret_proxy = xdg_surface;
                return TRUE;
            }
        }
    }

    if (strcmp(type, wl_surface_interface.name) == 0) {
        // We don't need to install a proxy here since we pretty much only want
        // to prevent commits until the first configure event has been ack'ed.
        if (opcode == WL_SURFACE_COMMIT) {
            GList *lock_surface_entry = g_list_find_custom (all_lock_surfaces, &proxy->object.id, find_lock_surface_by_id);
            if (lock_surface_entry) {
                SessionLockSurface *self = lock_surface_entry->data;
                if (!self->can_commit) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}
