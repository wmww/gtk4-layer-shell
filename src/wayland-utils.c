#include "wayland-utils.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <gdk/wayland/gdkwayland.h>
#include "wlr-layer-shell-unstable-v1-client.h"
#include <gtk/gtk.h>

static struct wl_registry* wl_registry_global = NULL;
static struct zwlr_layer_shell_v1* layer_shell_global = NULL;

static gboolean has_initialized = FALSE;

struct zwlr_layer_shell_v1* gtk_wayland_get_layer_shell_global() {
    return layer_shell_global;
}

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
        g_warn_if_fail(zwlr_layer_shell_v1_interface.version >= 3);
        layer_shell_global = wl_registry_bind(
            registry,
            id,
            &zwlr_layer_shell_v1_interface,
            MIN((uint32_t)zwlr_layer_shell_v1_interface.version, version)
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

void gtk_wayland_init_if_needed() {
    if (has_initialized) return;

    gtk_init();
    GdkDisplay* gdk_display = gdk_display_get_default();
    g_return_if_fail(gdk_display);
    g_return_if_fail(GDK_IS_WAYLAND_DISPLAY(gdk_display));

    struct wl_display* wl_display = gdk_wayland_display_get_wl_display(gdk_display);
    wl_registry_global = wl_display_get_registry(wl_display);
    wl_registry_add_listener(wl_registry_global, &wl_registry_listener, NULL);
    wl_display_roundtrip(wl_display);

    if (!layer_shell_global) {
        g_warning("it appears your Wayland compositor does not support the Layer Shell protocol");
    }

    has_initialized = TRUE;
}
