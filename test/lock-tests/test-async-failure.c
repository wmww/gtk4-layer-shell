#include "integration-test-common.h"

// Not part of the public API but we're the tests, we can do what we like
#include "../../src/registry.h"
#include "ext-session-lock-v1-client.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;

static void callback_0() {
    // We lock the display without going through the library in order to simulate a lock being held by another process
    GdkDisplay* gdk_display = gdk_display_get_default();
    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);
    struct ext_session_lock_manager_v1* global = get_session_lock_global_from_display(wl_display);
    struct ext_session_lock_v1* session_lock = ext_session_lock_manager_v1_lock(global);
    (void)session_lock;
}

static void callback_1() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .finished);

    lock = gtk_session_lock_instance_new();
    connect_lock_signals(lock, &state);

    ASSERT(gtk_session_lock_instance_lock(lock));
    ASSERT_EQ(state, LOCK_STATE_UNLOCKED, "%d");
    create_lock_windows(lock);
}

static void callback_2() {
    ASSERT_EQ(state, LOCK_STATE_FAILED, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
