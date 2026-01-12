#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;
GtkWindow* window;
static GtkWidget* popuper;

static void on_monitor(GtkSessionLockInstance* lock, GdkMonitor* monitor, void* data) {
    (void)lock; (void)monitor; (void)data;
    window = GTK_WINDOW(gtk_window_new());
    popuper = popup_widget_new();
    gtk_window_set_child(window, popuper);
    gtk_session_lock_instance_assign_window_to_monitor(lock, window, monitor);
}

static void callback_0() {
    lock = gtk_session_lock_instance_new();
    connect_lock_signals_except_monitor(lock, &state);
    g_signal_connect(lock, "monitor", G_CALLBACK(on_monitor), NULL);

    ASSERT(gtk_session_lock_instance_lock(lock));
}

static void callback_1() {
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");

    popup_widget_toggle_open(popuper);
}

static void callback_2() {
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");

    gtk_session_lock_instance_unlock(lock);
}

static void callback_3() {
    ASSERT_EQ(state, LOCK_STATE_UNLOCKED, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
