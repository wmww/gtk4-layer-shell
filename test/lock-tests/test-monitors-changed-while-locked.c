#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;

static void callback_0() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .configure 1920 1080);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .configure 640 480);

    send_command("create_output 640 480", "output_created");

    lock = gtk_session_lock_instance_new();
    connect_lock_signals(lock, &state);

    ASSERT(gtk_session_lock_instance_lock(lock));
}

static void callback_1() {
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");

    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .configure 1080 720);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .ack_configure);

    send_command("create_output 1080 720", "output_created");
}

static void callback_2() {
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);

    send_command("destroy_output 0", "output_destroyed");
}

static void callback_3() {
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);

    send_command("destroy_output 2", "output_destroyed");
}

static void callback_4() {
    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);

    gtk_session_lock_instance_unlock(lock);
}

static void callback_5() {
    ASSERT_EQ(state, LOCK_STATE_UNLOCKED, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
    callback_4,
    callback_5,
)
