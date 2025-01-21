#include "gtk4-session-lock.h"
#include "lock-surface.h"
#include "registry.h"
#include "libwayland-shim.h"

#include <gdk/wayland/gdkwayland.h>

static const char* lock_surface_key = "wayland_layer_surface";
static GList* all_lock_surfaces = NULL;

static struct ext_session_lock_manager_v1* init_and_get_session_lock_global() {
    gtk_init();
    GdkDisplay* gdk_display = gdk_display_get_default();
    g_return_val_if_fail(gdk_display, NULL);
    g_return_val_if_fail(GDK_IS_WAYLAND_DISPLAY(gdk_display), NULL);
    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);
    return get_session_lock_global_from_display(wl_display);
}

struct _GtkSessionLockSingleton {
    GObject parent_instance;
};

G_DEFINE_TYPE(GtkSessionLockSingleton, gtk_session_lock_singleton, G_TYPE_OBJECT)
static GtkSessionLockSingleton* session_lock_singleton = NULL;

enum {
    SESSION_LOCK_SIGNAL_LOCKED,
    SESSION_LOCK_SIGNAL_FINISHED,
    SESSION_LOCK_SIGNAL_LAST,
};

static guint session_lock_signals[SESSION_LOCK_SIGNAL_LAST] = {0};

static void gtk_session_lock_singleton_class_init(GtkSessionLockSingletonClass *cclass) {
    session_lock_signals[SESSION_LOCK_SIGNAL_LOCKED] = g_signal_new(
        "locked",
        G_TYPE_FROM_CLASS(cclass),
        G_SIGNAL_RUN_FIRST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        0
    );

    session_lock_signals[SESSION_LOCK_SIGNAL_FINISHED] = g_signal_new(
        "finished",
        G_TYPE_FROM_CLASS(cclass),
        G_SIGNAL_RUN_FIRST,
        0,
        NULL,
        NULL,
        NULL,
        G_TYPE_NONE,
        0
    );
}

static void gtk_session_lock_singleton_init(GtkSessionLockSingleton *self) {
    (void)self;
}

GtkSessionLockSingleton* gtk_session_lock_get_singleton() {
    if (!session_lock_singleton) {
        session_lock_singleton = g_object_new(gtk_session_lock_singleton_get_type(), NULL);
    }
    return session_lock_singleton;
}

static void session_lock_locked_callback_impl(bool locked) {
    g_signal_emit(
        gtk_session_lock_get_singleton(),
        session_lock_signals[locked ? SESSION_LOCK_SIGNAL_LOCKED : SESSION_LOCK_SIGNAL_FINISHED],
        0
    );
}

void gtk_session_lock_lock() {
    GdkDisplay* gdk_display = gdk_display_get_default();
    g_return_if_fail(gdk_display);
    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);
    g_return_if_fail(wl_display);
    session_lock_lock(wl_display, session_lock_locked_callback_impl);
}

void gtk_session_lock_unlock() {
    session_lock_unlock();
}

struct gtk_lock_surface_t {
    struct lock_surface_t super;
    GtkWindow* gtk_window;
};

static void gtk_lock_surface_destroy(struct gtk_lock_surface_t* self) {
    lock_surface_uninit(&self->super);
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
    lock_surface_map(&self->super);
}

void gtk_session_lock_assign_window_to_monitor(GtkWindow *window, GdkMonitor *monitor) {
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

    if (!libwayland_shim_has_initialized()) {
        g_warning("Failed to initialize lock surface, GTK4 Layer Shell may have been linked after libwayland.");
        g_message("Move gtk4-layer-shell before libwayland-client in the linker options.");
        g_message("You may be able to fix with without recompiling by setting LD_PRELOAD=/path/to/libgtk4-layer-shell.so");
        g_message("See https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md for more info");
        return;
    }

    if (!init_and_get_session_lock_global()) {
        g_warning("Failed to initialize layer surface, it appears your Wayland compositor doesn't support the Session Lock protocol");
        return;
    }

    static gboolean has_installed_hook = false;
    if (!has_installed_hook) {
        has_installed_hook = true;
        lock_surface_install_hook(lock_surface_hook_callback_impl);
    }

    struct gtk_lock_surface_t* lock_surface = g_new0(struct gtk_lock_surface_t, 1);
    lock_surface->gtk_window = window;
    lock_surface->super = lock_surface_make(output);
    g_object_set_data_full(G_OBJECT(window), lock_surface_key, lock_surface, (GDestroyNotify)gtk_lock_surface_destroy);
    g_signal_connect(window, "map", G_CALLBACK(on_window_mapped), lock_surface);

    all_lock_surfaces = g_list_append(all_lock_surfaces, lock_surface);

    gtk_window_set_decorated(window, FALSE);

    if (gtk_widget_get_mapped(GTK_WIDGET(window))) {
        gtk_widget_unrealize(GTK_WIDGET(window));
        gtk_widget_map(GTK_WIDGET(window));
    }
}
