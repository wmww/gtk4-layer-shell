#include "wayland-utils.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "wlr-layer-shell-unstable-v1-client.h"
#include <string.h>
#include <stdio.h>

static struct wl_display* cached_display = NULL;
static struct wl_registry* wl_registry_global = NULL;
static struct zwlr_layer_shell_v1* layer_shell_global = NULL;

static void wl_registry_handle_global(
    void* _data,
    struct wl_registry* registry,
    uint32_t id,
    const char* interface,
    uint32_t version
) {
    (void)_data;

    // pull out needed globals
    if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        uint32_t supported_version = zwlr_layer_shell_v1_interface.version;
        layer_shell_global = wl_registry_bind(
            registry,
            id,
            &zwlr_layer_shell_v1_interface,
            supported_version < version ? supported_version : version
        );
    }
}

static void wl_registry_handle_global_remove(
    void* _data,
    struct wl_registry* _registry,
    uint32_t _id
) {
    (void)_data;
    (void)_registry;
    (void)_id;
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_handle_global,
    .global_remove = wl_registry_handle_global_remove,
};

struct zwlr_layer_shell_v1* get_layer_shell_global_from_display(struct wl_display* display) {
    if (display != cached_display) {
        if (cached_display) {
            fprintf(stderr, "Wayland display changed, binding a new layer shell global\n");
        }
        cached_display = display;
        wl_registry_global = wl_display_get_registry(display);
        wl_registry_add_listener(wl_registry_global, &wl_registry_listener, NULL);
        wl_display_roundtrip(display);
        if (!layer_shell_global) {
            fprintf(stderr, "it appears your Wayland compositor does not support the Layer Shell protocol\n");
        }
    }
    return layer_shell_global;
}
