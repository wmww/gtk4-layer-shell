#include "lock-surface.h"

#include "libwayland-shim.h"
#include "stubbed-surface.h"
#include "session-lock.h"

#include <ext-session-lock-v1-client.h>
#include <wayland-client-protocol.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SURFACE_DATA_MAX_COUNT 100
static struct {
    struct wl_surface* wl_surface;
    bool has_buffer;
} surface_data[SURFACE_DATA_MAX_COUNT] = {0};
static int surface_data_count = 0;

static void lock_surface_handle_configure(
    void* data,
    struct ext_session_lock_surface_v1* surface,
    uint32_t serial,
    uint32_t width,
    uint32_t height
) {
    (void)surface;
    struct lock_surface_t* self = data;

    self->pending_configure_serial = serial;
    self->last_configure.width = width;
    self->last_configure.height = height;

    xdg_surface_server_send_configure(
        &self->super,
        0, 0,
        self->last_configure.width, self->last_configure.height,
        self->pending_configure_serial
    );
}

static const struct ext_session_lock_surface_v1_listener lock_surface_listener = {
    .configure = lock_surface_handle_configure,
};

static void lock_surface_configure_acked(struct xdg_surface_server_t* super, uint32_t serial) {
    struct lock_surface_t* self = (void*)super;
    if (serial && serial == self->pending_configure_serial) {
        self->pending_configure_serial = 0;
        ext_session_lock_surface_v1_ack_configure(self->lock_surface, serial);
    }
}

static void lock_surface_role_destroyed(struct xdg_surface_server_t* super) {
    struct lock_surface_t* self = (void*)super;
    if (self->lock_surface) {
        ext_session_lock_surface_v1_destroy(self->lock_surface);
        self->lock_surface = NULL;
    }
    self->pending_configure_serial = 0;
}

struct lock_surface_t lock_surface_make(struct wl_output* output) {
    struct lock_surface_t ret = {
        .super = {
            .configure_acked = lock_surface_configure_acked,
            .toplevel_destroyed = lock_surface_role_destroyed,
            .popup_destroyed = lock_surface_role_destroyed,
        },
        .output = output
    };
    return ret;
}

void lock_surface_map(struct lock_surface_t* self) {
    if (self->lock_surface) {
        return;
    }

    struct ext_session_lock_v1* current_lock = session_lock_get_active_lock();
    if (!current_lock) {
        fprintf(stderr, "failed to create lock surface, no current lock in place\n");
        return;
    }

    if (!self->super.wl_surface) {
        fprintf(stderr, "failed to create lock surface, no wl_surface set\n");
        return;
    }

    self->lock_surface = ext_session_lock_v1_get_lock_surface(current_lock, self->super.wl_surface, self->output);
    assert(self->lock_surface);
    ext_session_lock_surface_v1_add_listener(self->lock_surface, &lock_surface_listener, self);

    // Not strictly necessary, but roundtripping here will hopefully let us handle our initial configure before this
    // function returns, which may reduce the chance of GTK committing the surface before that initial configure for one
    // reasons or another (which is a protocol error).
    wl_display_roundtrip(libwayland_shim_proxy_get_display((struct wl_proxy*)self->super.wl_surface));
}

void lock_surface_uninit(struct lock_surface_t* self) {
    xdg_surface_server_uninit(&self->super);
}

static bool xdg_wm_base_get_xdg_surface_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data; (void)opcode; (void)create_interface; (void)create_version; (void)flags;

    lock_surface_hook_callback_t callback = data;
    struct wl_surface* wl_surface = (struct wl_surface*)args[1].o;
    struct lock_surface_t* self = callback(wl_surface);

    if (self) {
        *ret_proxy = xdg_surface_server_get_xdg_surface(&self->super, (struct xdg_wm_base*)proxy, wl_surface);
        assert(surface_data_count < SURFACE_DATA_MAX_COUNT);
        surface_data[surface_data_count].wl_surface = wl_surface;
        surface_data[surface_data_count].has_buffer = false;
        surface_data_count++;
        return true;
    } else if (session_lock_get_active_lock()) {
        // A new XDG surface is being created while the screen is locked, but it's not a lock surface. Few possibilities
        // 1. it's going to be a toplevel
        // 2. it's going to be a popup on a non-lock toplevel
        // 3. it's going to be a popup on a lock surface
        // For 1 and 2 it could be created as normal, but it wont be slown until the lock surface is closed. 3 is most
        // likely, but if we allow the creation of an XDG surface for it there's no easy way to avoid a protocol error.
        // Therefore we create a stub surface that won't display anything, but will keep GTK happy.
        fprintf(stderr, "non-lock surface created while screen locked, it will not displayed\n");
        *ret_proxy = stubbed_surface_init((struct xdg_wm_base*)proxy, wl_surface);
        return true;
    } {
        return false;
    }
}

static bool wl_surface_attach_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data; (void)opcode; (void)create_interface; (void)create_version; (void)flags; (void)ret_proxy;

    struct wl_buffer* buffer = (struct wl_buffer*)args[0].o;
    struct wl_surface* wl_surface = (struct wl_surface*)proxy;
    for (int i = 0; i < surface_data_count; i++) {
        if (surface_data[i].wl_surface == wl_surface) {
            if (buffer) {
                surface_data[i].has_buffer = true;
            } else {
                // This is a lock surface, so its not allowed to have a null buffer. return true to "handle" this
                // request, so the null buffer is not sent to the compositor.
                return true;
            }
        }
    }
    return false;
}

static bool wl_surface_commit_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data; (void)opcode; (void)create_interface; (void)create_version; (void)flags; (void)args; (void)ret_proxy;

    struct wl_surface* wl_surface = (struct wl_surface*)proxy;
    for (int i = 0; i < surface_data_count; i++) {
        if (surface_data[i].wl_surface == wl_surface) {
            if (!surface_data[i].has_buffer) {
                // We're not allowed to commit a lock surface without a buffer. return true to "handle" this request and
                // prevent it from being sent to the compositor
                return true;
            }
        }
    }
    return false;
}

static bool wl_surface_destroy_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data; (void)opcode; (void)create_interface; (void)create_version; (void)flags; (void)args; (void)ret_proxy;

    struct wl_surface* wl_surface = (struct wl_surface*)proxy;
    for (int i = 0; i < surface_data_count; i++) {
        if (surface_data[i].wl_surface == wl_surface) {
            memmove(
                &surface_data[i],
                &surface_data[i + 1],
                sizeof(surface_data[0]) * (surface_data_count - i - 1)
            );
            surface_data_count--;
            i--;
        }
    }
    return false;
}

void lock_surface_install_hook(lock_surface_hook_callback_t callback) {
    libwayland_shim_install_request_hook(
        &xdg_wm_base_interface,
        XDG_WM_BASE_GET_XDG_SURFACE,
        xdg_wm_base_get_xdg_surface_hook,
        callback
    );

    static bool hooks_installed = false;
    if (!hooks_installed) {
        hooks_installed = true;
        libwayland_shim_install_request_hook(
            &wl_surface_interface,
            WL_SURFACE_ATTACH,
            wl_surface_attach_hook,
            NULL
        );
        libwayland_shim_install_request_hook(
            &wl_surface_interface,
            WL_SURFACE_COMMIT,
            wl_surface_commit_hook,
            NULL
        );
        libwayland_shim_install_request_hook(
            &wl_surface_interface,
            WL_SURFACE_DESTROY,
            wl_surface_destroy_hook,
            NULL
        );
    }
}
