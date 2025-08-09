#include "gtk4-session-lock.h"
#include "session-lock.h"
#include "lock-surface.h"
#include "registry.h"
#include "libwayland-shim.h"

#include <gdk/wayland/gdkwayland.h>

static const char* lock_surface_key = "wayland_layer_surface";
static GList* all_lock_surfaces = NULL;

GTK4_LAYER_SHELL_EXPORT
gboolean gtk_session_lock_is_supported() {
    gtk_init();
    GdkDisplay* gdk_display = gdk_display_get_default();
    struct wl_display* wl_display = GDK_IS_WAYLAND_DISPLAY(gdk_display) ?
        gdk_wayland_display_get_wl_display(gdk_display) : NULL;
    struct ext_session_lock_manager_v1* global = wl_display ?
        get_session_lock_global_from_display(wl_display) : NULL;
    return global != NULL;
}

struct _GtkSessionLockInstance {
    GObject parent_instance;
    GListModel* monitors;
    void* wayland_object;
    gboolean is_locked;
    gboolean has_requested_lock;
    gboolean failed;
    GList* lock_surfaces;
};

struct gtk_lock_surface_t {
    struct lock_surface_t super;
    GtkWindow* gtk_window;
    GtkSessionLockInstance* lock;
};

G_DEFINE_TYPE(GtkSessionLockInstance, gtk_session_lock_instance, G_TYPE_OBJECT)

enum {
    SESSION_LOCK_SIGNAL_MONITOR,
    SESSION_LOCK_SIGNAL_LOCKED,
    SESSION_LOCK_SIGNAL_FAILED,
    SESSION_LOCK_SIGNAL_UNLOCKED,
    SESSION_LOCK_SIGNAL_LAST,
};

static guint session_lock_signals[SESSION_LOCK_SIGNAL_LAST] = {0};

static void gtk_session_lock_instance_class_init(GtkSessionLockInstanceClass *cclass) {
    session_lock_signals[SESSION_LOCK_SIGNAL_MONITOR] = g_signal_new(
        "monitor", G_TYPE_FROM_CLASS(cclass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, GDK_TYPE_MONITOR);

    session_lock_signals[SESSION_LOCK_SIGNAL_LOCKED] = g_signal_new(
        "locked", G_TYPE_FROM_CLASS(cclass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    session_lock_signals[SESSION_LOCK_SIGNAL_FAILED] = g_signal_new(
        "failed", G_TYPE_FROM_CLASS(cclass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    session_lock_signals[SESSION_LOCK_SIGNAL_UNLOCKED] = g_signal_new(
        "unlocked", G_TYPE_FROM_CLASS(cclass), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void gtk_session_lock_instance_init(GtkSessionLockInstance *self) {
    self->monitors = gdk_display_get_monitors(gdk_display_get_default());
    self->is_locked = FALSE;
    self->has_requested_lock = FALSE;
    self->failed = FALSE;
    self->lock_surfaces = NULL;
}

GTK4_LAYER_SHELL_EXPORT
GtkSessionLockInstance* gtk_session_lock_instance_new() {
    return g_object_new(gtk_session_lock_instance_get_type(), NULL);
}

static void monitors_changed(
    GListModel* monitors,
    guint position,
    guint removed,
    guint added,
    gpointer data
) {
    (void)removed;
    GtkSessionLockInstance* self = data;
    for (guint i = 0; i < added; i++) {
        g_signal_emit(
            self,
            session_lock_signals[SESSION_LOCK_SIGNAL_MONITOR],
            0,
            g_list_model_get_item(monitors, position + i)
        );
    }
}

static void start_monitor_signals(GtkSessionLockInstance* self) {
    guint n_monitors = g_list_model_get_n_items(self->monitors);
    for (guint i = 0; i < n_monitors; ++i) {
        g_signal_emit(
            self,
            session_lock_signals[SESSION_LOCK_SIGNAL_MONITOR],
            0,
            g_list_model_get_item(self->monitors, i)
        );
    }
    g_signal_connect(self->monitors, "items-changed", G_CALLBACK(monitors_changed), self);
}

static void clear_lock_state(GtkSessionLockInstance* self) {
    for (GList* item = self->lock_surfaces; item; item = item->next) {
        struct gtk_lock_surface_t* surface = item->data;
        gtk_widget_unrealize(GTK_WIDGET(surface->gtk_window));
        // This destroys GTK's internal reference to the window, the program could still be holding a reference to the
        // window if it wants to keep it alive and use it again.
        gtk_window_destroy(surface->gtk_window);
    }
    self->lock_surfaces = NULL;
    self->wayland_object = NULL;
    g_signal_handlers_disconnect_by_data(self->monitors, self);
}

static void session_lock_locked_callback_impl(bool locked, void* data) {
    GtkSessionLockInstance* self = data;
    if (!locked && !self->is_locked) {
        self->failed = TRUE;
    }
    self->is_locked = locked;
    if (!locked) {
        self->has_requested_lock = FALSE;
    }
    g_signal_emit(
        self,
        session_lock_signals[
            self->is_locked ? SESSION_LOCK_SIGNAL_LOCKED : (
                self->failed ? SESSION_LOCK_SIGNAL_FAILED : SESSION_LOCK_SIGNAL_UNLOCKED
            )
        ],
        0
    );
    if (!self->is_locked) {
        clear_lock_state(self);
    }
}

GTK4_LAYER_SHELL_EXPORT
gboolean gtk_session_lock_instance_lock(GtkSessionLockInstance* self) {
    if (self->has_requested_lock) {
        g_warning("Tried to lock multiple times without unlocking");
        return false;
    }

    GdkDisplay* gdk_display = gdk_display_get_default();
    struct wl_display* wl_display = GDK_IS_WAYLAND_DISPLAY(gdk_display) ?
        gdk_wayland_display_get_wl_display(gdk_display) :
        NULL;

    if (!wl_display) {
        g_signal_emit(self, session_lock_signals[SESSION_LOCK_SIGNAL_FAILED], 0);
        g_critical("Failed to get Wayland display");
        return false;
    }

    if (!get_session_lock_global_from_display(wl_display)) {
        g_critical("Session Lock protocol not supported");
        g_signal_emit(self, session_lock_signals[SESSION_LOCK_SIGNAL_FAILED], 0);
        return false;
    }

    if (!libwayland_shim_has_initialized()) {
        g_warning("Failed to initialize lock surface, GTK4 Layer Shell may have been linked after libwayland.");
        g_message("Move gtk4-layer-shell before libwayland-client in the linker options.");
        g_message("You may be able to fix with without recompiling by setting LD_PRELOAD=/path/to/libgtk4-layer-shell.so");
        g_message("See https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md for more info");
        g_signal_emit(self, session_lock_signals[SESSION_LOCK_SIGNAL_FAILED], 0);
        return false;
    }

    self->has_requested_lock = TRUE;
    session_lock_lock(wl_display, session_lock_locked_callback_impl, self);
    if (!self->failed) {
        self->wayland_object = session_lock_get_active_lock();
        start_monitor_signals(self);
    }
    return !self->failed;
}

GTK4_LAYER_SHELL_EXPORT
void gtk_session_lock_instance_unlock(GtkSessionLockInstance* self) {
    if (self->is_locked) {
        self->is_locked = FALSE;
        self->has_requested_lock = FALSE;
        g_signal_emit(self, session_lock_signals[SESSION_LOCK_SIGNAL_UNLOCKED], 0);
        session_lock_unlock();
        clear_lock_state(self);
    }
}

GTK4_LAYER_SHELL_EXPORT
gboolean gtk_session_lock_instance_is_locked(GtkSessionLockInstance* self) {
    return self->is_locked;
}

static void gtk_lock_surface_destroy(struct gtk_lock_surface_t* self) {
    lock_surface_uninit(&self->super);
    g_signal_handlers_disconnect_by_data(self->gtk_window, self);
    g_object_unref(self->lock);
    all_lock_surfaces = g_list_remove(all_lock_surfaces, self);
    g_free(self);
}

static gint find_lock_surface_with_wl_surface(gconstpointer lock_surface, gconstpointer needle) {
    struct gtk_lock_surface_t* self = (struct gtk_lock_surface_t*)lock_surface;
    GdkSurface* gdk_surface = gtk_native_get_surface(GTK_NATIVE(self->gtk_window));
    if (!gdk_surface) return 1;
    struct wl_surface* wl_surface = gdk_wayland_surface_get_wl_surface(gdk_surface);
    return wl_surface == needle ? 0 : 1;
}

static struct lock_surface_t* lock_surface_hook_callback_impl(struct wl_surface* wl_surface) {
    GList* entry = g_list_find_custom(
        all_lock_surfaces,
        wl_surface,
        find_lock_surface_with_wl_surface
    );
    return entry ? entry->data : NULL;
}

static void on_window_mapped(GtkWindow *window, gpointer data) {
    (void)window;
    struct gtk_lock_surface_t* self = data;
    if (self->lock->wayland_object == session_lock_get_active_lock()) {
        lock_surface_map(&self->super);
    } else {
        g_warning("Not showing lock surface because the session lock it is linked to is not active");
    }
}

GTK4_LAYER_SHELL_EXPORT
void gtk_session_lock_instance_assign_window_to_monitor(
    GtkSessionLockInstance* self,
    GtkWindow *window,
    GdkMonitor *monitor
) {
    if (!GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
        g_warning("Failed to initialize lock surface, not on Wayland");
        return;
    }

    if (!window) {
        g_warning("Failed to initialize lock surface, provided window is null");
        return;
    }

    if (!monitor) {
        g_warning("Failed to initialize lock surface, provided monitor is null");
        return;
    }

    struct wl_output* output = gdk_wayland_monitor_get_wl_output(monitor);
    if (!monitor) {
        g_warning("Failed to initialize lock surface, monitor does not have a wl_output");
        return;
    }

    static gboolean has_installed_hook = false;
    if (!has_installed_hook) {
        has_installed_hook = true;
        lock_surface_install_hook(lock_surface_hook_callback_impl);
    }

    struct gtk_lock_surface_t* lock_surface = g_new0(struct gtk_lock_surface_t, 1);
    lock_surface->lock = g_object_ref(self);
    lock_surface->gtk_window = window;
    lock_surface->super = lock_surface_make(output);
    g_object_set_data_full(G_OBJECT(window), lock_surface_key, lock_surface, (GDestroyNotify)gtk_lock_surface_destroy);
    g_signal_connect(window, "map", G_CALLBACK(on_window_mapped), lock_surface);

    all_lock_surfaces = g_list_append(all_lock_surfaces, lock_surface);
    self->lock_surfaces = g_list_append(self->lock_surfaces, lock_surface);

    gtk_window_set_decorated(window, FALSE);

    if (gtk_widget_get_realized(GTK_WIDGET(window))) {
        g_critical("gtk_session_lock_instance_assign_window_to_monitor() should not be called with an already realized window");
    }

    gtk_window_present(window);
}
