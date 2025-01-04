#include <dlfcn.h>
#include <stdlib.h>
#include "libwayland-shim.h"
#include "layer-surface.h"
#include "stolen-from-libwayland.h"

struct wl_proxy* (*libwayland_shim_real_wl_proxy_marshal_array_flags)(
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument* args
) = NULL;

void (*libwayland_shim_real_wl_proxy_destroy)(struct wl_proxy* proxy) = NULL;

int (*libwayland_shim_real_wl_proxy_add_dispatcher)(
    struct wl_proxy* proxy,
    wl_dispatcher_func_t dispatcher_func,
    const void* dispatcher_data, void* data
) = NULL;

gboolean libwayland_shim_has_initialized() {
    return libwayland_shim_real_wl_proxy_marshal_array_flags != NULL;
}

static void libwayland_shim_init() {
    if (libwayland_shim_has_initialized()) return;

    void* handle = dlopen("libwayland-client.so.0", RTLD_LAZY);
    if (handle == NULL) {
        handle = dlopen("libwayland-client.so", RTLD_LAZY);
    }
    if (handle == NULL) {
        g_error ("failed to dlopen libwayland");
    }

#define INIT_SYM(name) \
    if (!(libwayland_shim_real_##name = dlsym(handle, #name))) {\
        g_error("dlsym failed to load %s", #name); \
    }

    INIT_SYM(wl_proxy_marshal_array_flags);
    INIT_SYM(wl_proxy_destroy);
    INIT_SYM(wl_proxy_add_dispatcher);

#undef INIT_SYM

    //dlclose(handle);
}

struct wrapped_proxy {
    struct wl_proxy proxy;
    libwayland_shim_client_proxy_handler_func_t handler;
    libwayland_shim_client_proxy_destroy_func_t destroy;
    void* data;
};

// The ID for ALL proxies that are created by us and not managed by the real libwayland
const uint32_t client_facing_proxy_id = 6942069;

struct wl_proxy* libwayland_shim_create_client_proxy(
    struct wl_proxy* factory,
    const struct wl_interface* interface,
    uint32_t version,
    libwayland_shim_client_proxy_handler_func_t handler,
    libwayland_shim_client_proxy_destroy_func_t destroy,
    void* data
) {
    struct wrapped_proxy* allocation = g_new0(struct wrapped_proxy, 1);
    g_assert(allocation);
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
    g_assert(proxy->object.id == client_facing_proxy_id);
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
gboolean args_contains_client_facing_proxy(
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* interface,
    union wl_argument* args
) {
    (void)interface;
    const char* sig_iter = proxy->object.interface->methods[opcode].signature;
    int i = 0;
    while (true) {
        struct argument_details arg;
        sig_iter = get_next_argument(sig_iter, &arg);
        switch (arg.type) {
            case 'o':
                if (args[i].o && args[i].o->id == client_facing_proxy_id) {
                    return TRUE;
                }
                break;

            case '\0':
                return FALSE;
        }
        i++;
    }
}

// Returns the interface of the proxy object that this request is supposed to create, or NULL if none
const struct wl_interface* get_interface_of_object_created_by_request(
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* interface
) {
    const char* sig_iter = proxy->object.interface->methods[opcode].signature;
    int i = 0;
    while (true) {
        struct argument_details arg;
        sig_iter = get_next_argument(sig_iter, &arg);
        switch (arg.type) {
            case 'n':
                g_assert(interface[i].name);
                return interface + i;

            case '\0':
                return NULL;
        }
        i++;
    }
}

// Overrides the function in wayland-client.c in libwayland
void wl_proxy_destroy(struct wl_proxy* proxy) {
    libwayland_shim_init();
    if (proxy->object.id == client_facing_proxy_id) {
        struct wrapped_proxy* wrapper = (struct wrapped_proxy*)proxy;
        if (wrapper->destroy) {
            wrapper->destroy(wrapper->data, proxy);
        }
        wl_list_remove(&proxy->queue_link);
        // No need to worry about the refcount since it's only accessibly within libwayland, and it's only used by
        // functions that never see client facing objects
        g_free(proxy);
    } else {
        libwayland_shim_real_wl_proxy_destroy(proxy);
    }
}

// Overrides the function in wayland-client.c in libwayland
struct wl_proxy* wl_proxy_marshal_array_flags(
    struct wl_proxy* proxy,
    uint32_t opcode,
    const struct wl_interface* interface,
    uint32_t version,
    uint32_t flags,
    union wl_argument* args)
{
    libwayland_shim_init();
    if (proxy->object.id == client_facing_proxy_id) {
        // libwayland doesn't know about the object this request is on. It must not find out about this object. If it
        // finds out it will be very upset.
        struct wrapped_proxy* wrapper = (struct wrapped_proxy*)proxy;
        struct wl_proxy* result = NULL;
        if (wrapper->handler) {
            result = wrapper->handler(wrapper->data, proxy, opcode, interface, version, flags, args);
        }
        if (flags & WL_MARSHAL_FLAG_DESTROY) {
            wl_proxy_destroy(proxy);
        }
        return result;
    } else {
        struct wl_proxy* ret_proxy = NULL;
        const char* type_name = proxy->object.interface->name;
        if (layer_surface_handle_request(type_name, proxy, opcode, interface, version, flags, args, &ret_proxy)) {
            // The behavior of the request has been overridden
            return ret_proxy;
        } else if (args_contains_client_facing_proxy(proxy, opcode, interface, args)) {
            // We can't do the normal thing because one of the arguments is an object libwayand doesn't know about, but
            // no override behavior was taken. Hopefully we can safely ignore this request.
            const struct wl_interface* created = get_interface_of_object_created_by_request(proxy, opcode, interface);
            if (created) {
                // We need to create a stub object to make the client happy, it will ignore all requests and represents
                // nothing in libwayland/the server
               return libwayland_shim_create_client_proxy(proxy, created, created->version, NULL, NULL, NULL);
            } else {
                // Ignore the request
                return NULL;
            }
        } else {
            // Forward the request on to libwayland without modification, this is the most common path
            return libwayland_shim_real_wl_proxy_marshal_array_flags(proxy, opcode, interface, version, flags, args);
        }
    }
}

// Overrides the function in wayland-client.c in libwayland
int wl_proxy_add_dispatcher(
    struct wl_proxy* proxy,
    wl_dispatcher_func_t dispatcher_func,
    const void* dispatcher_data,
    void* data
) {
    libwayland_shim_init();
    if (proxy->object.id == client_facing_proxy_id) {
        g_critical("wl_proxy_add_dispatcher() not supported for client-facing proxies");
    }
    return libwayland_shim_real_wl_proxy_add_dispatcher(proxy, dispatcher_func, dispatcher_data, data);
}

void const* libwayland_shim_proxy_get_implementation(struct wl_proxy* proxy) {
    return proxy->object.implementation;
}

void* libwayland_shim_proxy_get_user_data(struct wl_proxy* proxy) {
    return proxy->user_data;
}
