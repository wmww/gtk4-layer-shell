#pragma once

#include <wayland-client-core.h>
#include <stdbool.h>

#define LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(listener, proxy, event, ...) \
    if (libwayland_shim_proxy_get_implementation((struct wl_proxy*)proxy)) { \
        ((struct listener*)libwayland_shim_proxy_get_implementation((struct wl_proxy*)proxy)) \
            ->event( \
                libwayland_shim_proxy_get_user_data((struct wl_proxy*)proxy), \
                __VA_ARGS__ \
            ); \
    }

typedef struct wl_proxy* (*libwayland_shim_client_proxy_handler_func_t)(
    void* data,
    struct wl_proxy *proxy,
    uint32_t opcode,
    const struct wl_interface *interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument *args);

typedef void (*libwayland_shim_client_proxy_destroy_func_t)(void* data, struct wl_proxy *proxy);

bool libwayland_shim_has_initialized();
struct wl_proxy* libwayland_shim_create_client_proxy(
    struct wl_proxy* factory,
    const struct wl_interface* interface,
    uint32_t version,
    libwayland_shim_client_proxy_handler_func_t handler,
    libwayland_shim_client_proxy_destroy_func_t destroy,
    void* data
);
void libwayland_shim_clear_client_proxy_data(struct wl_proxy* proxy);
void* libwayland_shim_get_client_proxy_data(struct wl_proxy* proxy, void* expected_handler);
void const* libwayland_shim_proxy_get_implementation(struct wl_proxy* proxy);
void* libwayland_shim_proxy_get_user_data(struct wl_proxy* proxy);
