#pragma once

#include <stdbool.h>
struct wl_display;

typedef void (*session_lock_locked_callback_t)(bool locked, void* data);
bool session_lock_lock(struct wl_display* display, session_lock_locked_callback_t callback, void* data);
struct ext_session_lock_v1* session_lock_get_active_lock();
void session_lock_unlock();
