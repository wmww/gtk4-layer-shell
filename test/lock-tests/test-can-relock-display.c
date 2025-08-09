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
    ASSERT(!gtk_session_lock_instance_is_locked(lock_a));

    ASSERT(gtk_session_lock_instance_lock(lock_a));
    create_default_lock_windows(lock_a);
}

static void callback_1() {
    ASSERT(gtk_session_lock_instance_is_locked(lock_a));
    ASSERT_EQ(state_a, LOCK_STATE_LOCKED, "%d");

    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);

    gtk_session_lock_instance_unlock(lock_a);
    ASSERT(!gtk_session_lock_instance_is_locked(lock_a));
    ASSERT_EQ(state_a, LOCK_STATE_UNLOCKED, "%d");
    CHECK_EXPECTATIONS();

    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .locked);

    ASSERT(gtk_session_lock_instance_lock(lock_a));
    create_default_lock_windows(lock_a);
}

static void callback_2() {
    ASSERT(gtk_session_lock_instance_is_locked(lock_a));
    ASSERT_EQ(state_a, LOCK_STATE_LOCKED, "%d");

    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);

    gtk_session_lock_instance_unlock(lock_a);
    ASSERT(!gtk_session_lock_instance_is_locked(lock_a));
    ASSERT_EQ(state_a, LOCK_STATE_UNLOCKED, "%d");
    CHECK_EXPECTATIONS();

    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .locked);

    lock_b = gtk_session_lock_instance_new();
    connect_lock_signals(lock_b, &state_b);

    ASSERT(gtk_session_lock_instance_lock(lock_b));
    create_default_lock_windows(lock_b);
}

static void callback_3() {
    ASSERT(gtk_session_lock_instance_is_locked(lock_b));
    ASSERT_EQ(state_b, LOCK_STATE_LOCKED, "%d");
    ASSERT(!gtk_session_lock_instance_is_locked(lock_a));
    ASSERT_EQ(state_a, LOCK_STATE_UNLOCKED, "%d");

    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);

    gtk_session_lock_instance_unlock(lock_b);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
