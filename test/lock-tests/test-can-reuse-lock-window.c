#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock = NULL;
GtkWindow* lock_window = NULL;

static GtkWindow* get_lock_window() {
    return lock_window;
}

static void callback_0() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .locked);

    lock_window = g_object_ref(create_default_window());

    lock = gtk_session_lock_instance_new();
    connect_lock_signals(lock, &state);
    ASSERT(!gtk_session_lock_instance_is_locked(lock));

    ASSERT(gtk_session_lock_instance_lock(lock));
    create_lock_windows(lock, get_lock_window);
}

static void callback_1() {
    ASSERT(gtk_session_lock_instance_is_locked(lock));
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");

    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);

    gtk_session_lock_instance_unlock(lock);
    ASSERT(!gtk_session_lock_instance_is_locked(lock));
    ASSERT_EQ(state, LOCK_STATE_UNLOCKED, "%d");
}

static void callback_2() {
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .locked);
    ASSERT(gtk_session_lock_instance_lock(lock));
    fprintf(stderr, "\nlock window: %p\n\n", lock_window);
    create_lock_windows(lock, get_lock_window);
    fprintf(stderr, "\nlock windows created\n\n");
}

static void callback_3() {
    ASSERT(gtk_session_lock_instance_is_locked(lock));
    ASSERT_EQ(state, LOCK_STATE_LOCKED, "%d");

    EXPECT_MESSAGE(ext_session_lock_v1 .unlock_and_destroy);
    EXPECT_MESSAGE(ext_session_lock_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);

    gtk_session_lock_instance_unlock(lock);
    ASSERT(!gtk_session_lock_instance_is_locked(lock));
    ASSERT_EQ(state, LOCK_STATE_UNLOCKED, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
