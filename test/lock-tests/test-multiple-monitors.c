#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;

static void callback_0() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .configure 1920 1080);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .configure 640 480);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .configure 1080 720);

    send_command("create_output 640 480", "output_created");
    send_command("create_output 1080 720", "output_created");

    lock = gtk_session_lock_instance_new();
    connect_lock_signals(lock, &state);

    ASSERT(gtk_session_lock_instance_lock(lock));
}

static void callback_1() {
    int number_of_toplevels = g_list_model_get_n_items(gtk_window_get_toplevels());
    ASSERT_EQ(number_of_toplevels, 3, "%d");
    bool found_0 = FALSE, found_1 = FALSE, found_2 = FALSE;
    for (int i = 0; i < number_of_toplevels; i++) {
        GtkWidget* toplevel = g_list_model_get_item(gtk_window_get_toplevels(), i);
        int w = gtk_widget_get_width(toplevel), h = gtk_widget_get_height(toplevel);
        if (w == DEFAULT_OUTPUT_WIDTH && h == DEFAULT_OUTPUT_HEIGHT) found_0 = TRUE;
        else if (w == 640 && h == 480) found_1 = TRUE;
        else if (w == 1080 && h == 720) found_2 = TRUE;
        else FATAL_FMT("toplevel window has invalid size %dx%d", w, h);
    }
    ASSERT(found_0);
    ASSERT(found_1);
    ASSERT(found_2);

    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");
    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);

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
