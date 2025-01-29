#include "session-lock.h"
#include "registry.h"

#include <ext-session-lock-v1-client.h>

struct wl_display* current_display = NULL;
struct ext_session_lock_v1* current_lock = NULL;
static session_lock_locked_callback_t current_callback = NULL;
static void* current_callback_data = NULL;
static bool is_locked = false;

static void session_lock_handle_locked(void* data, struct ext_session_lock_v1* session_lock) {
    (void)data;
    if (session_lock != current_lock) {
        ext_session_lock_v1_unlock_and_destroy(session_lock);
        return;
    }
    is_locked = true;
    if (current_callback) {
        current_callback(true, current_callback_data);
    }
}

static void session_lock_handle_finished(void* data, struct ext_session_lock_v1* session_lock) {
    (void)data;
    if (session_lock != current_lock) {
        ext_session_lock_v1_destroy(session_lock);
        return;
    }
    is_locked = false;
    if (current_callback) {
        current_callback(false, current_callback_data);
        current_callback = NULL;
        current_callback_data = NULL;
    }
}

static const struct ext_session_lock_v1_listener session_lock_listener = {
    .locked = session_lock_handle_locked,
    .finished = session_lock_handle_finished,
};

bool session_lock_lock(struct wl_display* display, session_lock_locked_callback_t callback, void* data) {
    if (current_lock) {
        callback(false, data);
        return false;
    }
    struct ext_session_lock_manager_v1* manager = get_session_lock_global_from_display(display);
    if (!manager) {
        callback(false, data);
        return false;
    }
    current_display = display;
    current_lock = ext_session_lock_manager_v1_lock(manager);
    current_callback = callback;
    current_callback_data = data;
    is_locked = false;
    ext_session_lock_v1_add_listener(current_lock, &session_lock_listener, NULL);
    return true;
}

struct ext_session_lock_v1* session_lock_get_active_lock() {
    return current_lock;
}

void session_lock_unlock() {
    if (!current_lock) return;
    if (is_locked) {
        ext_session_lock_v1_unlock_and_destroy(current_lock);
        wl_display_roundtrip(current_display);
    }
    // if there is a current lock that is not yet locked, nulling it will cause it to be destroyed when it gets a locked
    // or finished event
    current_display = NULL;
    current_lock = NULL;
    current_callback = NULL;
    current_callback_data = NULL;
    is_locked = false;
}
