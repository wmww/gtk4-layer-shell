#include "lock-surface.h"

#include "wayland-utils.h"
#include "libwayland-shim.h"

#include "xdg-shell-client.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct wl_display* current_display = NULL;
struct ext_session_lock_v1* current_lock = NULL;
static session_lock_locked_callback_t current_callback = NULL;
static bool is_locked = false;

static void session_lock_handle_locked(void* data, struct ext_session_lock_v1* session_lock) {
    (void)data;
    if (session_lock != current_lock) {
        ext_session_lock_v1_unlock_and_destroy(session_lock);
        return;
    }
    is_locked = true;
    if (current_callback) {
        current_callback(true);
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
        current_callback(false);
        current_callback = NULL;
    }
}

static const struct ext_session_lock_v1_listener session_lock_listener = {
    .locked = session_lock_handle_locked,
    .finished = session_lock_handle_finished,
};

void session_lock_lock(struct wl_display* display, session_lock_locked_callback_t callback) {
    if (current_lock) {
        callback(false);
        return;
    }
    struct ext_session_lock_manager_v1* manager = get_session_lock_global_from_display(display);
    if (!manager) {
        callback(false);
        return;
    }
    current_display = display;
    current_lock = ext_session_lock_manager_v1_lock(manager);
    current_callback = callback;
    is_locked = false;
    ext_session_lock_v1_add_listener(current_lock, &session_lock_listener, callback);
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
    is_locked = false;
}

static void lock_surface_send_xdg_configure(struct lock_surface_t* self) {
    struct wl_array states;
    wl_array_init(&states); {
        uint32_t* state = wl_array_add(&states, sizeof(uint32_t));
        assert(state);
        *state = XDG_TOPLEVEL_STATE_ACTIVATED;
    } {
        uint32_t* state = wl_array_add(&states, sizeof(uint32_t));
        assert(state);
        *state = XDG_TOPLEVEL_STATE_FULLSCREEN;
    }

    LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
        xdg_toplevel_listener,
        configure,
        self->client_facing_xdg_toplevel,
        self->last_configure.width, self->last_configure.height,
        &states);
    wl_array_release(&states);

    LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(
        xdg_surface_listener,
        configure,
        self->client_facing_xdg_surface,
        self->pending_configure_serial);
}

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

    lock_surface_send_xdg_configure(self);
}

static const struct ext_session_lock_surface_v1_listener lock_surface_listener = {
    .configure = lock_surface_handle_configure,
};

struct lock_surface_t lock_surface_make(struct wl_output* output) {
    struct lock_surface_t ret = {
        .output = output
    };
    return ret;
}

void lock_surface_map(struct lock_surface_t* self) {
    if (self->lock_surface) {
        return;
    }

    if (!current_lock) {
        fprintf(stderr, "failed to create lock surface, no current lock in place\n");
        return;
    }

    if (!self->wl_surface) {
        fprintf(stderr, "failed to create lock surface, no wl_surface set\n");
        return;
    }

    self->lock_surface = ext_session_lock_v1_get_lock_surface(current_lock, self->wl_surface, self->output);
    assert(self->lock_surface);
    ext_session_lock_surface_v1_add_listener(self->lock_surface, &lock_surface_listener, self);
}

static void lock_surface_unmap(struct lock_surface_t* self) {
    if (self->lock_surface) {
        ext_session_lock_surface_v1_destroy(self->lock_surface);
        self->lock_surface = NULL;
    }

    libwayland_shim_clear_client_proxy_data((struct wl_proxy*)self->client_facing_xdg_surface);
    libwayland_shim_clear_client_proxy_data((struct wl_proxy*)self->client_facing_xdg_toplevel);

    self->client_facing_xdg_surface = NULL;
    self->client_facing_xdg_toplevel = NULL;
    self->pending_configure_serial = 0;
}

void lock_surface_uninit(struct lock_surface_t* self) {
    lock_surface_unmap(self);
}

static void client_xdg_toplevel_handle_destroy(void* data, struct wl_proxy* proxy) {
    (void)proxy;
    struct lock_surface_t* self = (struct lock_surface_t*)data;
    lock_surface_unmap(self);
}

static bool client_xdg_surface_handle_request(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)interface; (void)flags;
    struct lock_surface_t* self = (struct lock_surface_t*)data;
    if (opcode == XDG_SURFACE_GET_TOPLEVEL) {
        *ret_proxy = libwayland_shim_create_client_proxy(
            proxy,
            &xdg_toplevel_interface,
            version,
            NULL,
            client_xdg_toplevel_handle_destroy,
            data
        );
        self->client_facing_xdg_toplevel = (struct xdg_toplevel*)*ret_proxy;
        return true;
    } else if (opcode == XDG_SURFACE_GET_POPUP) {
        fprintf(stderr, "XDG surface intercepted, but is now being used as popup\n");
        *ret_proxy = libwayland_shim_create_client_proxy(proxy, &xdg_popup_interface, version, NULL, NULL, NULL);
        return true;
    } else if (opcode == XDG_SURFACE_SET_WINDOW_GEOMETRY) {
        return false;
    } else if (opcode == XDG_SURFACE_ACK_CONFIGURE) {
        uint32_t serial = args[0].u;
        if (serial && serial == self->pending_configure_serial) {
            self->pending_configure_serial = 0;
            ext_session_lock_surface_v1_ack_configure(self->lock_surface, serial);
        }
        return false;
    } else {
        return false;
    }
}

static void client_xdg_surface_handle_destroy(void* data, struct wl_proxy* proxy) {
    (void)proxy;
    struct lock_surface_t* self = (struct lock_surface_t*)data;
    lock_surface_unmap(self);
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
    (void)data;
    (void)opcode;
    (void)create_interface;
    (void)flags;

    lock_surface_hook_callback_t callback = data;
    struct wl_surface* wl_surface = (struct wl_surface*)args[1].o;
    struct lock_surface_t* self = callback(wl_surface);

    if (self) {
        self->wl_surface = wl_surface;
        struct wl_proxy* xdg_surface = libwayland_shim_create_client_proxy(
            proxy,
            &xdg_surface_interface,
            create_version,
            client_xdg_surface_handle_request,
            client_xdg_surface_handle_destroy,
            self
        );
        self->client_facing_xdg_surface = (struct xdg_surface*)xdg_surface;
        *ret_proxy = xdg_surface;
        return true;
    } else {
        return false;
    }
}

static bool xdg_surface_get_popup_hook(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy
) {
    (void)data;
    (void)opcode;
    (void)create_interface;
    (void)flags;

    struct lock_surface_t* self = libwayland_shim_get_client_proxy_data(
        (struct wl_proxy*)args[1].o,
        client_xdg_surface_handle_request
    );

    if (self) {
        fprintf(stderr, "popups not supported for session lock surfaces\n");
        *ret_proxy = libwayland_shim_create_client_proxy(
            proxy,
            &xdg_popup_interface,
            create_version,
            NULL, NULL, NULL
        );
        return true;
    } else {
        return false;
    }
}

void lock_surface_install_hook(lock_surface_hook_callback_t callback) {
    libwayland_shim_install_request_hook(
        &xdg_wm_base_interface,
        XDG_WM_BASE_GET_XDG_SURFACE,
        xdg_wm_base_get_xdg_surface_hook,
        callback
    );

    static bool popup_hook_initialized = false;
    if (!popup_hook_initialized) {
        popup_hook_initialized = true;
        libwayland_shim_install_request_hook(
            &xdg_surface_interface,
            XDG_SURFACE_GET_POPUP,
            xdg_surface_get_popup_hook,
            NULL
        );
    }
}
