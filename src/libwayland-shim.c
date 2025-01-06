#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "libwayland-shim.h"
#include "stolen-from-libwayland.h"

bool layer_surface_handle_request(
    const char* type_name,
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* created_interface,
    uint32_t created_version,
    uint32_t flags,
    union wl_argument* args,
    struct wl_proxy **ret_proxy
);

static bool has_initialized = NULL;

static struct wl_proxy* (*real_wl_proxy_marshal_array_flags)(
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* created_interface,
    uint32_t created_version,
    uint32_t flags,
    union wl_argument* args
) = NULL;

static void (*real_wl_proxy_destroy)(struct wl_proxy* proxy) = NULL;

bool libwayland_shim_has_initialized() {
    return has_initialized;
}

static void libwayland_shim_init() {
    if (has_initialized) return;

#define INIT_SYM(name) \
    if (!(real_##name = dlsym(RTLD_NEXT, #name))) {\
        fprintf(stderr, "libwayland_shim: dlsym failed to load %s\n", #name); \
        exit(1); \
    }

    INIT_SYM(wl_proxy_marshal_array_flags);
    INIT_SYM(wl_proxy_destroy);

#undef INIT_SYM

    has_initialized = true;
}

struct wrapped_proxy {
    struct wl_proxy proxy;
    libwayland_shim_request_handler_func_t handler;
    libwayland_shim_destroy_handler_func_t destroy;
    void* data;
};

// The ID for ALL proxies that are created by us and not managed by the real libwayland
const uint32_t client_facing_proxy_id = 6942069;

struct wl_proxy* libwayland_shim_create_client_proxy(
    struct wl_proxy* factory,
    const struct wl_interface* interface,
    uint32_t version,
    libwayland_shim_request_handler_func_t handler,
    libwayland_shim_destroy_handler_func_t destroy,
    void* data
) {
    struct wrapped_proxy* allocation = calloc(1, sizeof(struct wrapped_proxy));
    assert(allocation);
    allocation->proxy.object.interface = interface;
    allocation->proxy.display = factory->display;
    allocation->proxy.queue = factory->queue;
    allocation->proxy.refcount = 1;
    allocation->proxy.version = version;
    allocation->proxy.object.id = client_facing_proxy_id;
    wl_list_init(&allocation->proxy.queue_link);
    allocation->handler = handler;
    allocation->destroy = destroy;
    allocation->data = data;
    return &allocation->proxy;
}

void libwayland_shim_clear_client_proxy_data(struct wl_proxy* proxy) {
    if (!proxy) return;
    assert(proxy->object.id == client_facing_proxy_id);
    struct wrapped_proxy* wrapper = (struct wrapped_proxy*)proxy;
    wrapper->data = NULL;
    wrapper->destroy = NULL;
    wrapper->handler = NULL;
}

void* libwayland_shim_get_client_proxy_data(struct wl_proxy* proxy, void* expected_handler) {
    if (proxy && proxy->object.id == client_facing_proxy_id) {
        struct wrapped_proxy* wrapper = (struct wrapped_proxy*)proxy;
        if (wrapper->handler == expected_handler) {
            return wrapper->data;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

// Returns true if any arguments are proxies created by us(not known to libwayland)
static bool args_contains_client_facing_proxy(
    struct wl_proxy* proxy,
    uint32_t opcode,
    union wl_argument* args
) {
    const char* sig_iter = proxy->object.interface->methods[opcode].signature;
    int i = 0;
    while (true) {
        struct argument_details arg;
        sig_iter = get_next_argument(sig_iter, &arg);
        switch (arg.type) {
            case 'o':
                if (args[i].o && args[i].o->id == client_facing_proxy_id) {
                    return true;
                }
                break;

            case '\0':
                return false;
        }
        i++;
    }
}

static void client_proxy_destroy(struct wl_proxy* proxy) {
    struct wrapped_proxy* wrapper = (struct wrapped_proxy*)proxy;
    if (wrapper->destroy) {
        wrapper->destroy(wrapper->data, proxy);
    }
    wl_list_remove(&proxy->queue_link);
    // No need to worry about the refcount since it's only accessibly within libwayland, and it's only used by
    // functions that never see client facing objects
    free(proxy);
}

// Overrides the function in wayland-client.c in libwayland
void wl_proxy_destroy(struct wl_proxy* proxy) {
    libwayland_shim_init();
    if (proxy->object.id == client_facing_proxy_id) {
        client_proxy_destroy(proxy);
    } else {
        real_wl_proxy_destroy(proxy);
    }
}

static struct wl_proxy* fallback_handle_request(
    struct wl_proxy* proxy,
    struct wl_interface const* create_interface,
    uint32_t create_version
) {
    if (create_interface) {
        // We need to create a stub object to make the client happy, it will ignore all requests and represents
        // nothing in libwayland/the server
        return libwayland_shim_create_client_proxy(proxy, create_interface, create_version, NULL, NULL, NULL);
    } else {
        // The request does not create an object
        return NULL;
    }
}

static struct wl_proxy* validate_request_result(
    struct wl_proxy* created_proxy,
    struct wl_proxy* request_proxy,
    uint32_t opcode,
    struct wl_interface const* create_interface,
    uint32_t create_version
) {
    if (create_interface) {
        if (!created_proxy) {
            fprintf(
                stderr,
                "libwayland_shim: request %s.%s should have created object of type %s, but handler created nothing\n",
                request_proxy->object.interface->name,
                request_proxy->object.interface->methods[opcode].name,
                create_interface->name
            );
            return fallback_handle_request(request_proxy, create_interface, create_version);
        } else if (strcmp(created_proxy->object.interface->name, create_interface->name) != 0) {
            fprintf(
                stderr,
                "libwayland_shim: request %s.%s should have created object of type %s, but handler created object of type %s\n",
                request_proxy->object.interface->name,
                request_proxy->object.interface->methods[opcode].name,
                create_interface->name,
                created_proxy->object.interface->name
            );
            wl_proxy_destroy(created_proxy);
            return fallback_handle_request(request_proxy, create_interface, create_version);
        } else {
            return created_proxy;
        }
    } else {
        if (created_proxy) {
            fprintf(
                stderr,
                "libwayland_shim: request %s.%s should not have created anything, but handler created object of type %s\n",
                request_proxy->object.interface->name,
                request_proxy->object.interface->methods[opcode].name,
                created_proxy->object.interface->name
            );
            wl_proxy_destroy(created_proxy);
        }
        return NULL;
    }
}

// Overrides the function in wayland-client.c in libwayland, handles requests made by the client program and optionally
// forwards them to libwayland/the compositor
struct wl_proxy* wl_proxy_marshal_array_flags(
    struct wl_proxy* proxy,
    uint32_t opcode,
    struct wl_interface const* create_interface,
    uint32_t create_version,
    uint32_t flags,
    union wl_argument* args)
{
    libwayland_shim_init();
    if (proxy->object.id == client_facing_proxy_id) {
        // libwayland doesn't know about the object this request is on. It must not find out about this object. If it
        // finds out it will be very upset.
        struct wrapped_proxy* wrapper = (struct wrapped_proxy*)proxy;
        bool handled = false;
        struct wl_proxy* ret_proxy = NULL;
        if (wrapper->handler) {
            // Call the custom request handler
            if (wrapper->handler(
                wrapper->data,
                proxy,
                opcode,
                create_interface,
                create_version,
                flags,
                args,
                &ret_proxy
            )) {
                handled = true;
                ret_proxy = validate_request_result(ret_proxy, proxy, opcode, create_interface, create_version);
            }
        }

        if (flags & WL_MARSHAL_FLAG_DESTROY) {
            // The caller has indicated this request destroys the proxy
            client_proxy_destroy(proxy);
        }

        return handled ? ret_proxy : fallback_handle_request(proxy, create_interface, create_version);
    } else {
        struct wl_proxy* ret_proxy = NULL;
        const char* type_name = proxy->object.interface->name;
        if (layer_surface_handle_request(
            type_name,
            proxy,
            opcode,
            create_interface,
            create_version,
            flags,
            args,
            &ret_proxy)
        ) {
            // The behavior of the request has been overridden
            return validate_request_result(ret_proxy, proxy, opcode, create_interface, create_version);
        } else if (args_contains_client_facing_proxy(proxy, opcode, args)) {
            // We can't do the normal thing because one of the arguments is an object libwayand doesn't know about, but
            // no override behavior was taken. Hopefully we can safely ignore this request.
            return fallback_handle_request(proxy, create_interface, create_version);
        } else {
            // Forward the request on to libwayland without modification, this is the most common path
            return real_wl_proxy_marshal_array_flags(proxy, opcode, create_interface, create_version, flags, args);
        }
    }
}

wl_dispatcher_func_t libwayland_shim_proxy_get_dispatcher(struct wl_proxy* proxy) {
    return proxy->dispatcher;
}

void libwayland_shim_proxy_invoke_dispatcher(struct wl_proxy* proxy, uint32_t opcode, ...) {
    // This should be possible to implement if needed, but not much seems to use dispatchers
    fprintf(
        stderr,
        "libwayland_shim: invoking event %s@%d.%s: dispatchers not currently supported for client objects\n",
        proxy->object.interface->name,
        proxy->object.id,
        proxy->object.interface->methods[opcode].name
    );
}

void const* libwayland_shim_proxy_get_implementation(struct wl_proxy* proxy) {
    return proxy->object.implementation;
}

void* libwayland_shim_proxy_get_user_data(struct wl_proxy* proxy) {
    return proxy->user_data;
}
