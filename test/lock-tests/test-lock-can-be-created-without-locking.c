#include "integration-test-common.h"

static void callback_0() {
    UNEXPECT_MESSAGE(ext_session_lock_v1 .lock);

    GtkSessionLockInstance* lock = gtk_session_lock_instance_new();
    g_object_unref(lock);
}

TEST_CALLBACKS(
    callback_0,
)
