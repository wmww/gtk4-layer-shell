#include "integration-test-common.h"

// Regression test for https://github.com/wmww/gtk4-layer-shell/issues/122
// Unlike the other lock tests, the lock window here is owned by a GtkApplication. Destroying such a window emits
// GtkApplication::window-removed, and on GTK >= 4.22 gtk_application_window_removed() uses the window's GdkSurface
// (for XDG session management), so the window must still be realized when gtk_window_destroy() is called.
// Unrealizing it first crashed with a use-after-free on every unlock or lock failure.

enum lock_state_t state = 0;
GtkApplication* app;
GtkSessionLockInstance* lock;

static void on_monitor(GtkSessionLockInstance* lock_, GdkMonitor* monitor, void* data) {
    (void)data;
    GtkWindow* window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_window_set_child(window, gtk_label_new("locked"));
    gtk_session_lock_instance_assign_window_to_monitor(lock_, window, monitor);
}

static void callback_0() {
    app = gtk_application_new("com.github.wmww.gtk4-layer-shell.test", G_APPLICATION_DEFAULT_FLAGS);
    ASSERT(g_application_register(G_APPLICATION(app), NULL, NULL));

    lock = gtk_session_lock_instance_new();
    connect_lock_signals_except_monitor(lock, &state);
    g_signal_connect(lock, "monitor", G_CALLBACK(on_monitor), NULL);
    ASSERT(gtk_session_lock_instance_lock(lock));
}

static void callback_1() {
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");
    gtk_session_lock_instance_unlock(lock);
}

static void callback_2() {
    ASSERT_EQ(state, LOCK_STATE_UNLOCKED, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
