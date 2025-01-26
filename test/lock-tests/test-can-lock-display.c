#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;

static void callback_0() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .locked);

    lock = gtk_session_lock_instance_new();
    connect_lock_signals(lock, &state);

    ASSERT(gtk_session_lock_instance_lock(lock));
    create_lock_windows(lock);
}

static void callback_1() {
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");
    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);

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
