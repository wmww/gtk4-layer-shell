#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;
GtkWindow* window;
static GtkWidget *dropdown;
static const char *options[] = {"Foo", "Bar", "Baz", NULL};

GtkWindow* build_window() {
    window = GTK_WINDOW(gtk_window_new());
    dropdown = gtk_drop_down_new_from_strings(options);
    gtk_window_set_child(window, dropdown);
    return window;
}

static void callback_0() {
    // The popup is weirdly slow to open, so slow the tests down
    step_time = 1200;

    lock = gtk_session_lock_instance_new();
    connect_lock_signals(lock, &state);

    ASSERT(gtk_session_lock_instance_lock(lock));
    create_lock_windows(lock, build_window);
}

static void callback_1() {
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");

    g_signal_emit_by_name(dropdown, "activate", NULL);
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
