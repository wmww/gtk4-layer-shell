#include "registry.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "wlr-layer-shell-unstable-v1-client.h"
#include "ext-session-lock-v1-client.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

struct wl_display* cached_display = NULL;

bool layer_shell_requested = false;
struct zwlr_layer_shell_v1* layer_shell_global = NULL;

bool session_lock_requested = false;
struct ext_session_lock_manager_v1* session_lock_global = NULL;

bool subcompositor_requested = false;
struct wl_subcompositor* subcompositor_global = NULL;

static void wl_registry_handle_global(
    void* _data,
    struct wl_registry* registry,
    uint32_t id,
    const char* interface,
    uint32_t version
) {
    (void)_data;

    // pull out needed globals
    if (subcompositor_requested && strcmp(interface, wl_subcompositor_interface.name) == 0) {
        uint32_t supported_version = wl_subcompositor_interface.version;
        subcompositor_global = wl_registry_bind(
            registry,
            id,
            &wl_subcompositor_interface,
            supported_version < version ? supported_version : version
        );
    } else if (layer_shell_requested && strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        uint32_t supported_version = zwlr_layer_shell_v1_interface.version;
        layer_shell_global = wl_registry_bind(
            registry,
            id,
            &zwlr_layer_shell_v1_interface,
            supported_version < version ? supported_version : version
        );
    } else if (session_lock_requested && strcmp(interface, ext_session_lock_manager_v1_interface.name) == 0) {
        uint32_t supported_version = ext_session_lock_manager_v1_interface.version;
        session_lock_global = wl_registry_bind(
            registry,
            id,
            &ext_session_lock_manager_v1_interface,
            supported_version < version ? supported_version : version
        );
    }
}

static void wl_registry_handle_global_remove(void* _data, struct wl_registry* _registry, uint32_t _id) {
    (void)_data;
    (void)_registry;
    (void)_id;
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_handle_global,
    .global_remove = wl_registry_handle_global_remove,
};

static void bind_globals(struct wl_display* display) {
    if (cached_display && display != cached_display) {
        fprintf(stderr, "Wayland display changed, binding a new layer shell global\n");
        cached_display = display;
        if (layer_shell_global) {
            zwlr_layer_shell_v1_destroy(layer_shell_global);
            layer_shell_global = NULL;
        }
        if (session_lock_global) {
            ext_session_lock_manager_v1_destroy(session_lock_global);
            session_lock_global = NULL;
        }
    }
    struct wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &wl_registry_listener, NULL);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
}

struct zwlr_layer_shell_v1* get_layer_shell_global_from_display(struct wl_display* display) {
    if (!layer_shell_requested) {
        layer_shell_requested = true;
        bind_globals(display);
        if (!layer_shell_global) {
            fprintf(stderr, "it appears your Wayland compositor does not support the Session Lock protocol\n");
        }
    }
    return layer_shell_global;
}

struct ext_session_lock_manager_v1* get_session_lock_global_from_display(struct wl_display* display) {
    if (!session_lock_requested) {
        session_lock_requested = true;
        subcompositor_requested = true; // If session lock is in use the subcompositor will likely be needed
        bind_globals(display);
        if (!session_lock_global) {
            fprintf(stderr, "it appears your Wayland compositor does not support the Session Lock protocol\n");
        }
    }
    return session_lock_global;
}

struct wl_subcompositor* get_subcompositor_global_from_display(struct wl_display* display) {
    if (!subcompositor_requested) {
        subcompositor_requested = true;
        bind_globals(display);
    }
    return subcompositor_global;
}
