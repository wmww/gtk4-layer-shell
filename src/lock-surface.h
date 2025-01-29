#pragma once

#include "xdg-surface-server.h"

struct lock_surface_t {
    struct xdg_surface_server_t super;
    struct ext_session_lock_surface_v1* lock_surface;
    struct wl_output* output;
    uint32_t pending_configure_serial;
    struct {
        int width, height;
    } last_configure;
};

struct lock_surface_t lock_surface_make(struct wl_output* output);
void lock_surface_map(struct lock_surface_t* self);
void lock_surface_uninit(struct lock_surface_t* self);

// Each time the current process attempts to create a new xdg_surface out of a wl_surface this callback will be called.
// If the given callback returns a non-null pointer, this lock surface is used to override the XDG surface. Else the
// XDG surface is created normally. Thus must be used for any lock surfaces to be displayed.
typedef struct lock_surface_t* (*lock_surface_hook_callback_t)(struct wl_surface*);
void lock_surface_install_hook(lock_surface_hook_callback_t callback);
