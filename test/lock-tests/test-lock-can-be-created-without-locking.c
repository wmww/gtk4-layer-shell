#include "integration-test-common.h"

enum lock_state_t state = 0;
GtkSessionLockInstance* lock;

static void callback_0() {
    UNEXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    UNEXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    UNEXPECT_MESSAGE(ext_session_lock_v1 .locked);

    ASSERT(gtk_session_lock_is_supported());

    lock = gtk_session_lock_instance_new();
    connect_lock_signals(lock, &state);
    g_object_unref(lock);

    ASSERT_EQ(state, LOCK_STATE_UNLOCKED, "%d");
}

TEST_CALLBACKS(
    callback_0,
)
