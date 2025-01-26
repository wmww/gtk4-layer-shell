#include "mock-server.h"
#include "linux/input.h"

typedef enum {
    SURFACE_ROLE_NONE = 0,
    SURFACE_ROLE_XDG_TOPLEVEL,
    SURFACE_ROLE_XDG_POPUP,
    SURFACE_ROLE_LAYER,
    SURFACE_ROLE_SESSION_LOCK,
} SurfaceRole;

typedef struct SurfaceData SurfaceData;

struct SurfaceData {
    SurfaceRole role;
    struct wl_resource* surface;
    struct wl_resource* pending_frame;
    struct wl_resource* pending_buffer; // The attached but not committed buffer
    char buffer_cleared; // If the buffer has been explicitly cleared since the last commit
    struct wl_resource* xdg_toplevel;
    struct wl_resource* xdg_popup;
    struct wl_resource* xdg_surface;
    struct wl_resource* layer_surface;
    struct wl_resource* lock_surface;
    char has_committed_buffer; // This surface has a non-null committed buffer
    char initial_commit_for_role; // Set to 1 when a role is created for a surface, and cleared after the first commit
    char layer_send_configure; // If to send a layer surface configure on the next commit
    int layer_set_w; // The width to configure the layer surface with
    int layer_set_h; // The height to configure the layer surface with
    uint32_t layer_anchor; // The layer surface's anchor
    uint32_t lock_surface_pending_serial;
    char lock_surface_initial_configure_acked;
    SurfaceData* most_recent_popup; // Start of the popup linked list
    SurfaceData* previous_popup_sibling; // Forms a linked list of popups
    SurfaceData* popup_parent;
};

static struct wl_resource* seat_global = NULL;
static struct wl_resource* pointer_global = NULL;
static struct wl_resource* output_global = NULL;
static struct wl_resource* current_session_lock = NULL;

static void surface_data_assert_no_role(SurfaceData* data) {
    ASSERT(!data->xdg_popup);
    ASSERT(!data->xdg_toplevel);
    ASSERT(!data->xdg_surface);
    ASSERT(!data->layer_surface);
    ASSERT(!data->lock_surface);
}

// Needs to be called before any role objects are assigned
static void surface_data_set_role(SurfaceData* data, SurfaceRole role) {
    if (data->role != SURFACE_ROLE_NONE) {
        ASSERT_EQ(data->role, role, "%u");
    }

    if (role == SURFACE_ROLE_XDG_TOPLEVEL || role == SURFACE_ROLE_XDG_POPUP) {
        ASSERT(data->xdg_surface != NULL);
    } else {
        ASSERT(data->xdg_surface == NULL);
    }

    struct wl_resource* xdg_surface = data->xdg_surface;
    data->xdg_surface = NULL; // XDG surfaces are allowed, so hide it from surface_data_assert_no_role()
    surface_data_assert_no_role(data);
    data->xdg_surface = xdg_surface;

    ASSERT(!data->has_committed_buffer);
    data->role = role;
    data->initial_commit_for_role = 1;
}

static void surface_data_unmap(SurfaceData* data) {
    SurfaceData* popup = data->most_recent_popup;
    while (popup) {
        // Popups must be unmapped before their parents
        surface_data_assert_no_role(data);
        popup = popup->previous_popup_sibling;
    }
}

static void surface_data_add_pupup(SurfaceData* parent, SurfaceData* popup) {
    ASSERT(!popup->previous_popup_sibling);
    popup->previous_popup_sibling = parent->most_recent_popup;
    parent->most_recent_popup = popup;
    popup->popup_parent = parent;
}

static void wl_surface_frame(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(callback, 0);
    SurfaceData* data = wl_resource_get_user_data(resource);
    ASSERT(!data->pending_frame);
    data->pending_frame = wl_resource_create(
        wl_resource_get_client(resource),
        &wl_callback_interface,
        wl_resource_get_version(resource),
        callback);
}

static void wl_surface_attach(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    RESOURCE_ARG(wl_buffer, buffer, 0);
    SurfaceData* data = wl_resource_get_user_data(resource);
    data->pending_buffer = buffer;
    data->buffer_cleared = buffer == NULL;
}

static void wl_surface_commit(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    SurfaceData* data = wl_resource_get_user_data(resource);

    if (data->role == SURFACE_ROLE_SESSION_LOCK) {
        if (data->buffer_cleared) {
            FATAL("null buffer committed to session lock surface");
        } else if (!data->pending_buffer && !data->has_committed_buffer) {
            FATAL("no buffer has been attached to committed session lock surface");
        } else if (!data->lock_surface_initial_configure_acked) {
            FATAL("session lock surface committed before initial configure acked");
        }
    }

    if (data->buffer_cleared) {
        data->has_committed_buffer = 0;
        data->buffer_cleared = 0;
    } else if (data->pending_buffer) {
        data->has_committed_buffer = 1;
    }

    if (data->pending_buffer) {
        wl_buffer_send_release(data->pending_buffer);
        data->pending_buffer = NULL;
    }

    if (data->pending_frame) {
        wl_callback_send_done(data->pending_frame, 0);
        wl_resource_destroy(data->pending_frame);
        data->pending_frame = NULL;
    }

    if (data->initial_commit_for_role && data->role != SURFACE_ROLE_SESSION_LOCK) {
        ASSERT(!data->has_committed_buffer);
        data->initial_commit_for_role = 0;
    }

    if (data->layer_surface && data->layer_send_configure) {
        char horiz = (
            (data->layer_anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) &&
            (data->layer_anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT));
        char vert = (
            (data->layer_anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP) &&
            (data->layer_anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM));
        int width = data->layer_set_w;
        int height = data->layer_set_h;
        if (width == 0 && !horiz)
            FATAL("not horizontally stretched and no width given");
        if (height == 0 && !vert)
            FATAL("not horizontally stretched and no width given");
        if (horiz)
            width = DEFAULT_OUTPUT_WIDTH;
        if (vert)
            height = DEFAULT_OUTPUT_HEIGHT;
        zwlr_layer_surface_v1_send_configure(data->layer_surface, wl_display_next_serial(display), width, height);
        data->layer_send_configure = 0;
    }
}

static void wl_surface_destroy(struct wl_resource* resource, const struct wl_message* message, union wl_argument* args) {
    SurfaceData* data = wl_resource_get_user_data(resource);
    surface_data_assert_no_role(data);
    data->surface = NULL;
    // Don't free surfaces to guarantee traversing popups is always safe
    // We're employing the missile memory management pattern here https://x.com/pomeranian99/status/858856994438094848
}

static void wl_compositor_create_surface(struct wl_resource* resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    struct wl_resource* surface = wl_resource_create(
        wl_resource_get_client(resource),
        &wl_surface_interface,
        wl_resource_get_version(resource),
        id);
    SurfaceData* data = ALLOC_STRUCT(SurfaceData);
    data->surface = surface;
    use_default_impl(surface);
    wl_resource_set_user_data(surface, data);
}

void wl_seat_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    ASSERT(!seat_global);
    seat_global = wl_resource_create(client, &wl_seat_interface, version, id);
    use_default_impl(seat_global);
    wl_seat_send_capabilities(seat_global, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
};

void wl_output_bind(struct wl_client* client, void* data, uint32_t version, uint32_t id) {
    ASSERT(!output_global);
    output_global = wl_resource_create(client, &wl_output_interface, version, id);
    use_default_impl(output_global);
    wl_output_send_done(output_global);
};

static void wl_seat_get_pointer(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    ASSERT(!pointer_global);
    pointer_global = wl_resource_create(
        wl_resource_get_client(resource),
        &wl_pointer_interface,
        wl_resource_get_version(resource),
        id);
    use_default_impl(pointer_global);
}

static void xdg_wm_base_get_xdg_surface(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    RESOURCE_ARG(wl_surface, surface, 1);
    struct wl_resource* xdg_surface = wl_resource_create(
        wl_resource_get_client(resource),
        &xdg_surface_interface,
        wl_resource_get_version(resource),
        id);
    use_default_impl(xdg_surface);
    SurfaceData* data = wl_resource_get_user_data(surface);
    wl_resource_set_user_data(xdg_surface, data);
    data->xdg_surface = xdg_surface;
}

static void xdg_surface_destroy(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    SurfaceData* data = wl_resource_get_user_data(resource);
    ASSERT(!data->xdg_toplevel);
    ASSERT(!data->xdg_popup);
    data->xdg_surface = NULL;
}

static void xdg_surface_get_toplevel(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    struct wl_resource* toplevel = wl_resource_create(
        wl_resource_get_client(resource),
        &xdg_toplevel_interface,
        wl_resource_get_version(resource),
        id);
    use_default_impl(toplevel);
    struct wl_array states;
    wl_array_init(&states);
    xdg_toplevel_send_configure(toplevel, 0, 0, &states);
    wl_array_release(&states);
    xdg_surface_send_configure(resource, wl_display_next_serial(display));
    SurfaceData* data = wl_resource_get_user_data(resource);
    surface_data_set_role(data, SURFACE_ROLE_XDG_TOPLEVEL);
    wl_resource_set_user_data(toplevel, data);
    data->xdg_toplevel = toplevel;
}

static void xdg_toplevel_destroy(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    SurfaceData* data = wl_resource_get_user_data(resource);
    ASSERT(data->xdg_surface);
    data->xdg_toplevel = NULL;
    surface_data_unmap(data);
}

static void xdg_surface_get_popup(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    RESOURCE_ARG(xdg_surface, parent, 1);
    struct wl_resource* popup = wl_resource_create(
        wl_resource_get_client(resource),
        &xdg_popup_interface,
        wl_resource_get_version(resource),
        id);
    use_default_impl(popup);
    // If the configure size is too small GTK gets upset and unmaps its popup in protest
    // https://gitlab.gnome.org/GNOME/gtk/-/blob/4.16.12/gtk/gtkpopover.c?ref_type=tags#L719
    xdg_popup_send_configure(popup, 0, 0, 500, 500);
    xdg_surface_send_configure(resource, wl_display_next_serial(display));
    SurfaceData* data = wl_resource_get_user_data(resource);
    surface_data_set_role(data, SURFACE_ROLE_XDG_POPUP);
    wl_resource_set_user_data(popup, data);
    data->xdg_popup = popup;
    if (parent) {
        SurfaceData* parent_data = wl_resource_get_user_data(parent);
        surface_data_add_pupup(parent_data, data);
    }
}

static void xdg_popup_grab(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    SurfaceData* data = wl_resource_get_user_data(resource);
    RESOURCE_ARG(wl_seat, seat, 0);
    ASSERT_EQ(seat, seat_global, "%p");
    ASSERT(data->popup_parent);
}

static void xdg_popup_destroy(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    SurfaceData* data = wl_resource_get_user_data(resource);
    ASSERT(data->xdg_surface);
    data->xdg_popup = NULL;
    surface_data_unmap(data);
}

static void zwlr_layer_surface_v1_set_anchor(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    UINT_ARG(anchor, 0);
    SurfaceData* data = wl_resource_get_user_data(resource);
    data->layer_send_configure = 1;
    data->layer_anchor = anchor;
}

static void zwlr_layer_surface_v1_set_size(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    UINT_ARG(width, 0);
    UINT_ARG(height, 1);
    SurfaceData* data = wl_resource_get_user_data(resource);
    data->layer_send_configure = 1;
    data->layer_set_w = width;
    data->layer_set_h = height;
}

static void zwlr_layer_surface_v1_get_popup(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    RESOURCE_ARG(xdg_popup, popup, 0);
    SurfaceData* data = wl_resource_get_user_data(resource);
    SurfaceData* popup_data = wl_resource_get_user_data(popup);
    ASSERT(!popup_data->popup_parent);
    surface_data_add_pupup(data, popup_data);
}

static void zwlr_layer_shell_v1_get_layer_surface(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    RESOURCE_ARG(wl_surface, surface, 1);
    struct wl_resource* layer_surface = wl_resource_create(
        wl_resource_get_client(resource),
        &zwlr_layer_surface_v1_interface,
        wl_resource_get_version(resource),
        id);
    use_default_impl(layer_surface);
    SurfaceData* data = wl_resource_get_user_data(surface);
    surface_data_set_role(data, SURFACE_ROLE_LAYER);
    wl_resource_set_user_data(layer_surface, data);
    data->layer_send_configure = 1;
    data->layer_surface = layer_surface;
}

static void zwlr_layer_surface_v1_destroy(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    SurfaceData* data = wl_resource_get_user_data(resource);
    data->layer_surface = NULL;
    surface_data_unmap(data);
}

static void ext_session_lock_manager_v1_lock(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    struct wl_resource* lock_resource = wl_resource_create(
        wl_resource_get_client(resource),
        &ext_session_lock_v1_interface,
        wl_resource_get_version(resource),
        id);
    use_default_impl(lock_resource);
    if (current_session_lock) {
        ext_session_lock_v1_send_finished(lock_resource);
    } else {
        current_session_lock = lock_resource;
        ext_session_lock_v1_send_locked(lock_resource);
    }
}

static void ext_session_lock_v1_destroy(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    if (resource == current_session_lock) {
        FATAL(".destroy (instead of .unlock_and_destroy) called on active lock");
    }
}

static void ext_session_lock_v1_unlock_and_destroy(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    if (resource != current_session_lock) {
        FATAL(".unlock_and_destroy (instead of .destroy) called on inactive lock");
    }
    current_session_lock = NULL;
}

static void lock_surface_send_configure(SurfaceData* data, uint32_t width, uint32_t height) {
    data->lock_surface_pending_serial = wl_display_next_serial(display);
    ext_session_lock_surface_v1_send_configure(data->lock_surface, data->lock_surface_pending_serial, width, height);
}

static void ext_session_lock_v1_get_lock_surface(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    NEW_ID_ARG(id, 0);
    struct wl_resource* lock_surface = wl_resource_create(
        wl_resource_get_client(resource),
        &ext_session_lock_surface_v1_interface,
        wl_resource_get_version(resource),
        id);
    use_default_impl(lock_surface);
    RESOURCE_ARG(wl_surface, surface, 1);
    RESOURCE_ARG(wl_output, output, 2);
    ASSERT_EQ(output, output_global, "%p");
    SurfaceData* data = wl_resource_get_user_data(surface);
    surface_data_set_role(data, SURFACE_ROLE_SESSION_LOCK);
    wl_resource_set_user_data(lock_surface, data);
    data->lock_surface = lock_surface;
    lock_surface_send_configure(data, DEFAULT_OUTPUT_WIDTH, DEFAULT_OUTPUT_HEIGHT);
}

static void ext_session_lock_surface_v1_ack_configure(struct wl_resource *resource, const struct wl_message* message, union wl_argument* args) {
    UINT_ARG(serial, 0);
    SurfaceData* data = wl_resource_get_user_data(resource);
    if (serial == data->lock_surface_pending_serial) {
        data->lock_surface_initial_configure_acked = 1;
    }
}

void init() {
    OVERRIDE_REQUEST(wl_surface, commit);
    OVERRIDE_REQUEST(wl_surface, frame);
    OVERRIDE_REQUEST(wl_surface, attach);
    OVERRIDE_REQUEST(wl_surface, destroy);
    OVERRIDE_REQUEST(wl_compositor, create_surface);
    OVERRIDE_REQUEST(wl_seat, get_pointer);
    OVERRIDE_REQUEST(xdg_wm_base, get_xdg_surface);
    OVERRIDE_REQUEST(xdg_surface, destroy);
    OVERRIDE_REQUEST(xdg_surface, get_toplevel);
    OVERRIDE_REQUEST(xdg_toplevel, destroy);
    OVERRIDE_REQUEST(xdg_surface, get_popup);
    OVERRIDE_REQUEST(xdg_popup, grab);
    OVERRIDE_REQUEST(xdg_popup, destroy);
    OVERRIDE_REQUEST(zwlr_layer_shell_v1, get_layer_surface);
    OVERRIDE_REQUEST(zwlr_layer_surface_v1, set_anchor);
    OVERRIDE_REQUEST(zwlr_layer_surface_v1, set_size);
    OVERRIDE_REQUEST(zwlr_layer_surface_v1, get_popup);
    OVERRIDE_REQUEST(zwlr_layer_surface_v1, destroy);
    OVERRIDE_REQUEST(ext_session_lock_manager_v1, lock);
    OVERRIDE_REQUEST(ext_session_lock_v1, destroy);
    OVERRIDE_REQUEST(ext_session_lock_v1, unlock_and_destroy);
    OVERRIDE_REQUEST(ext_session_lock_v1, get_lock_surface);
    OVERRIDE_REQUEST(ext_session_lock_surface_v1, ack_configure);

    wl_global_create(display, &wl_seat_interface, 6, NULL, wl_seat_bind);
    wl_global_create(display, &wl_output_interface, 2, NULL, wl_output_bind);
    default_global_create(display, &wl_shm_interface, 1);
    default_global_create(display, &wl_data_device_manager_interface, 2);
    default_global_create(display, &wl_compositor_interface, 4);
    default_global_create(display, &wl_subcompositor_interface, 1);
    default_global_create(display, &xdg_wm_base_interface, 2);
    default_global_create(display, &zwlr_layer_shell_v1_interface, 4);
    default_global_create(display, &ext_session_lock_manager_v1_interface, 1);
    default_global_create(display, &xdg_wm_dialog_v1_interface, 1);
}
