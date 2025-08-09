#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;

static void callback_0() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .locked);

    lock = gtk_session_lock_instance_new();
    connect_lock_signals_except_monitor(lock, &state);
    ASSERT(gtk_session_lock_instance_lock(lock));
    GListModel* monitors = gdk_display_get_monitors(gdk_display_get_default());
    ASSERT_EQ(g_list_model_get_n_items(monitors), 1, "%d");
    GdkMonitor* monitor = g_list_model_get_item(monitors, 0);
    GtkWindow* window = create_default_window();
    gtk_session_lock_instance_assign_window_to_monitor(lock, window, monitor);

    // This isn't needed, but it was recommend in an earlier version of the library so we test that it works
    gtk_window_present(window);
}

static void callback_1() {
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");
    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    gtk_session_lock_instance_unlock(lock);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
