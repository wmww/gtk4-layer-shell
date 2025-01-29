#pragma once

#include <wayland-client-core.h>
#include <stdbool.h>

// Function type for optionally overriding the behavior of requests from the client program (our process). *ret_proxy
// always starts as NULL. If true is returned this request is considered handled and *ret_proxy must be either left as
// NULL or set to a newly created proxy (whichever is required by the specific request being handled). If false is
// returned libwayland_shim falls back to it's default behavior, which may create a stub proxy to return if needed.
typedef bool (*libwayland_shim_request_handler_func_t)(
    void* data,
    struct wl_proxy* proxy,
    uint32_t opcode,
    struct wl_interface const* created_interface,
    uint32_t created_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy** ret_proxy);

// Function type for handling the destruction of client proxies
typedef void (*libwayland_shim_destroy_handler_func_t)(void* data, struct wl_proxy* proxy);

// Detect if library has correctly initialized. This should happen automatically when something connects to a Wayland
// display. If this is false it may indicate the program was linked incorrectly.
bool libwayland_shim_has_initialized();

// The handler will be called and can optionally handle all matching requests. There is no way to uninstall a hook, and
// installing too many hooks may hurt performance.
void libwayland_shim_install_request_hook(
    struct wl_interface const* interface,
    uint32_t opcode,
    libwayland_shim_request_handler_func_t handler,
    void* data
);

// Create a "fake" wl_proxy the client program (our process) can interact with, but libwayland doesn't know about
// factory: the proxy this proxy was created from (for example an xdg_surface if the created proxy is an xdg_toplevel)
// interface: the type of this proxy, these structs are declared in wayland client headers
// version: the version of the created proxy object
// handler: will be called for each request the client program (our process) makes
// destroy: will be called when the proxy is destroyed
// data: user data handler() and destroy() are called with, the caller is responsible for memory management
struct wl_proxy* libwayland_shim_create_client_proxy(
    struct wl_proxy* factory,
    struct wl_interface const* interface,
    uint32_t version,
    libwayland_shim_request_handler_func_t handler,
    libwayland_shim_destroy_handler_func_t destroy,
    void* data
);

// May only be called on NULL (no-op) or proxies created with libwayland_shim_create_client_proxy(). The proxy remains
// valid but handler and destory will no longer be called. If this function is used destroy is never called.
void libwayland_shim_clear_client_proxy_data(struct wl_proxy* proxy);

// Get the user data for a given client proxy, returns NULL unless the given proxy is a client proxy and its handler is
// the given expected handler
void* libwayland_shim_get_client_proxy_data(struct wl_proxy* proxy, void* expected_handler);

// Yes wl_proxy_get_display exists, but only in libwayland >=1.23. This can be replaced with that once we can drop
// support for older libwaylands (though no rush, this works)
struct wl_display* libwayland_shim_proxy_get_display(struct wl_proxy* proxy);
// Same deal, wl_proxy_get_queue() exists but is new
struct wl_event_queue* libwayland_shim_proxy_get_queue(struct wl_proxy* proxy);

#define LIBWAYLAND_SHIM_FIRST_ARG(first, ...) first
#define LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT(listener, event, ...) \
    if (libwayland_shim_proxy_get_dispatcher((struct wl_proxy*)LIBWAYLAND_SHIM_FIRST_ARG(__VA_ARGS__))) { \
        libwayland_shim_proxy_invoke_dispatcher( \
            (long)((char*)((struct listener*)NULL)->event) / sizeof(void(*)()), \
            (struct wl_proxy*)__VA_ARGS__ \
        ); \
    } else if (libwayland_shim_proxy_get_implementation((struct wl_proxy*)LIBWAYLAND_SHIM_FIRST_ARG(__VA_ARGS__))) { \
        ((struct listener*)libwayland_shim_proxy_get_implementation((struct wl_proxy*)LIBWAYLAND_SHIM_FIRST_ARG(__VA_ARGS__))) \
            ->event( \
                libwayland_shim_proxy_get_user_data((struct wl_proxy*)LIBWAYLAND_SHIM_FIRST_ARG(__VA_ARGS__)), \
                __VA_ARGS__ \
            ); \
    }

// These functions are used by LIBWAYLAND_SHIM_DISPATCH_CLIENT_EVENT()
wl_dispatcher_func_t libwayland_shim_proxy_get_dispatcher(struct wl_proxy* proxy);
void libwayland_shim_proxy_invoke_dispatcher(uint32_t opcode, struct wl_proxy* proxy, ...);
void const* libwayland_shim_proxy_get_implementation(struct wl_proxy* proxy);
void* libwayland_shim_proxy_get_user_data(struct wl_proxy* proxy);
