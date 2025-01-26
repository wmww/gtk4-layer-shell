#include "integration-test-common.h"

enum lock_state_t state_a = 0;
GtkSessionLockInstance* lock_a;

enum lock_state_t state_b = 0;
GtkSessionLockInstance* lock_b;

static void callback_0() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .locked);

    lock_a = gtk_session_lock_instance_new();
    connect_lock_signals(lock_a, &state_a);

    lock_b = gtk_session_lock_instance_new();
    connect_lock_signals(lock_b, &state_b);

    ASSERT(gtk_session_lock_instance_lock(lock_b));
    create_lock_windows(lock_b);
}

static void callback_1() {
    ASSERT_EQ(state_a, LOCK_STATE_UNLOCKED, "%d");
    ASSERT_EQ(state_b, LOCK_STATE_LOCKED, "%d");
    UNEXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    UNEXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    UNEXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    UNEXPECT_MESSAGE(ext_session_lock_v1 .destroy);
    UNEXPECT_MESSAGE(ext_session_lock_v1 .locked);
    UNEXPECT_MESSAGE(ext_session_lock_v1 .finished);

    ASSERT(!gtk_session_lock_instance_lock(lock_a));
    ASSERT_EQ(state_a, LOCK_STATE_FAILED, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
