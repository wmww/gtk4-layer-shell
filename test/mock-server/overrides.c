#include "mock-server.h"
#include "linux/input.h"

enum surface_role_t {
    SURFACE_ROLE_NONE = 0,
    SURFACE_ROLE_XDG_TOPLEVEL,
    SURFACE_ROLE_XDG_POPUP,
    SURFACE_ROLE_LAYER,
    SURFACE_ROLE_SESSION_LOCK,
};

struct surface_data_t {
    enum surface_role_t role;
    struct wl_resource* surface;
    struct wl_resource* pending_frame;
    struct wl_resource* pending_buffer; // The attached but not committed buffer
    bool buffer_cleared; // If the buffer has been explicitly cleared since the last commit
    struct wl_resource* xdg_toplevel;
    struct wl_resource* xdg_popup;
    struct wl_resource* xdg_surface;
    struct wl_resource* layer_surface;
    struct wl_resource* lock_surface;
    bool has_committed_buffer; // This surface has a non-null committed buffer
    bool initial_commit_for_role; // Set to 1 when a role is created for a surface, and cleared after the first commit
    bool layer_send_configure; // If to send a layer surface configure on the next commit
    int layer_set_w; // The width to configure the layer surface with
    int layer_set_h; // The height to configure the layer surface with
    uint32_t layer_anchor; // The layer surface's anchor
    uint32_t lock_surface_pending_serial;
    bool lock_surface_initial_configure_acked;
    struct surface_data_t* most_recent_popup; // Start of the popup linked list
    struct surface_data_t* previous_popup_sibling; // Forms a linked list of popups
    struct surface_data_t* popup_parent;
};

static struct wl_resource* seat_global = NULL;
static struct wl_resource* pointer_global = NULL;
static struct wl_resource* output_global = NULL;
static struct wl_resource* current_session_lock = NULL;

static void surface_data_assert_no_role(struct surface_data_t* data) {
    ASSERT(!data->xdg_popup);
    ASSERT(!data->xdg_toplevel);
    ASSERT(!data->xdg_surface);
    ASSERT(!data->layer_surface);
    ASSERT(!data->lock_surface);
}

// Needs to be called before any role objects are assigned
static void surface_data_set_role(struct surface_data_t* data, enum surface_role_t role) {
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

static void surface_data_unmap(struct surface_data_t* data) {
    struct surface_data_t* popup = data->most_recent_popup;
    while (popup) {
        // Popups must be unmapped before their parents
        surface_data_assert_no_role(data);
        popup = popup->previous_popup_sibling;
    }
}

static void surface_data_add_pupup(struct surface_data_t* parent, struct surface_data_t* popup) {
    ASSERT(!popup->previous_popup_sibling);
    popup->previous_popup_sibling = parent->most_recent_popup;
    parent->most_recent_popup = popup;
    popup->popup_parent = parent;
}

REQUEST_OVERRIDE_IMPL(wl_surface, frame) {
    struct surface_data_t* data = wl_resource_get_user_data(wl_surface);
    ASSERT(!data->pending_frame);
    data->pending_frame = new_resource;
}

REQUEST_OVERRIDE_IMPL(wl_surface, attach) {
    RESOURCE_ARG(wl_buffer, buffer, 0);
    struct surface_data_t* data = wl_resource_get_user_data(wl_surface);
    data->pending_buffer = buffer;
    data->buffer_cleared = buffer == NULL;
}

REQUEST_OVERRIDE_IMPL(wl_surface, commit) {
    struct surface_data_t* data = wl_resource_get_user_data(wl_surface);

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
        bool horiz = (
            (data->layer_anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT) &&
            (data->layer_anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT));
        bool vert = (
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

REQUEST_OVERRIDE_IMPL(wl_surface, destroy) {
    struct surface_data_t* data = wl_resource_get_user_data(wl_surface);
    surface_data_assert_no_role(data);
    data->surface = NULL;
    // Don't free surfaces to guarantee traversing popups is always safe
    // We're employing the missile memory management pattern here https://x.com/pomeranian99/status/858856994438094848
}

REQUEST_OVERRIDE_IMPL(wl_compositor, create_surface) {
    struct surface_data_t* data = calloc(1, sizeof(struct surface_data_t));
    wl_resource_set_user_data(new_resource, data);
    data->surface = new_resource;
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

REQUEST_OVERRIDE_IMPL(wl_seat, get_pointer) {
    ASSERT(!pointer_global);
    pointer_global = new_resource;
}

REQUEST_OVERRIDE_IMPL(xdg_wm_base, get_xdg_surface) {
    RESOURCE_ARG(wl_surface, surface, 1);
    struct surface_data_t* data = wl_resource_get_user_data(surface);
    wl_resource_set_user_data(new_resource, data);
    data->xdg_surface = new_resource;
}

REQUEST_OVERRIDE_IMPL(xdg_surface, destroy) {
    struct surface_data_t* data = wl_resource_get_user_data(xdg_surface);
    ASSERT(!data->xdg_toplevel);
    ASSERT(!data->xdg_popup);
    data->xdg_surface = NULL;
}

REQUEST_OVERRIDE_IMPL(xdg_surface, get_toplevel) {
    struct wl_array states;
    wl_array_init(&states);
    xdg_toplevel_send_configure(new_resource, 0, 0, &states);
    wl_array_release(&states);
    xdg_surface_send_configure(xdg_surface, wl_display_next_serial(display));
    struct surface_data_t* data = wl_resource_get_user_data(xdg_surface);
    surface_data_set_role(data, SURFACE_ROLE_XDG_TOPLEVEL);
    wl_resource_set_user_data(new_resource, data);
    data->xdg_toplevel = new_resource;
}

REQUEST_OVERRIDE_IMPL(xdg_toplevel, destroy) {
    struct surface_data_t* data = wl_resource_get_user_data(xdg_toplevel);
    ASSERT(data->xdg_surface);
    data->xdg_toplevel = NULL;
    surface_data_unmap(data);
}

REQUEST_OVERRIDE_IMPL(xdg_surface, get_popup) {
    RESOURCE_ARG(xdg_surface, parent, 1);
    // If the configure size is too small GTK gets upset and unmaps its popup in protest
    // https://gitlab.gnome.org/GNOME/gtk/-/blob/4.16.12/gtk/gtkpopover.c?ref_type=tags#L719
    xdg_popup_send_configure(new_resource, 0, 0, 500, 500);
    xdg_surface_send_configure(xdg_surface, wl_display_next_serial(display));
    struct surface_data_t* data = wl_resource_get_user_data(xdg_surface);
    surface_data_set_role(data, SURFACE_ROLE_XDG_POPUP);
    wl_resource_set_user_data(new_resource, data);
    data->xdg_popup = new_resource;
    if (parent) {
        struct surface_data_t* parent_data = wl_resource_get_user_data(parent);
        surface_data_add_pupup(parent_data, data);
    }
}

REQUEST_OVERRIDE_IMPL(xdg_popup, grab) {
    struct surface_data_t* data = wl_resource_get_user_data(xdg_popup);
    RESOURCE_ARG(wl_seat, seat, 0);
    ASSERT_EQ(seat, seat_global, "%p");
    ASSERT(data->popup_parent);
}

REQUEST_OVERRIDE_IMPL(xdg_popup, destroy) {
    struct surface_data_t* data = wl_resource_get_user_data(xdg_popup);
    ASSERT(data->xdg_surface);
    data->xdg_popup = NULL;
    surface_data_unmap(data);
}

REQUEST_OVERRIDE_IMPL(zwlr_layer_surface_v1, set_anchor) {
    UINT_ARG(anchor, 0);
    struct surface_data_t* data = wl_resource_get_user_data(zwlr_layer_surface_v1);
    data->layer_send_configure = 1;
    data->layer_anchor = anchor;
}

REQUEST_OVERRIDE_IMPL(zwlr_layer_surface_v1, set_size) {
    UINT_ARG(width, 0);
    UINT_ARG(height, 1);
    struct surface_data_t* data = wl_resource_get_user_data(zwlr_layer_surface_v1);
    data->layer_send_configure = 1;
    data->layer_set_w = width;
    data->layer_set_h = height;
}

REQUEST_OVERRIDE_IMPL(zwlr_layer_surface_v1, get_popup) {
    RESOURCE_ARG(xdg_popup, popup, 0);
    struct surface_data_t* data = wl_resource_get_user_data(zwlr_layer_surface_v1);
    struct surface_data_t* popup_data = wl_resource_get_user_data(popup);
    ASSERT(!popup_data->popup_parent);
    surface_data_add_pupup(data, popup_data);
}

REQUEST_OVERRIDE_IMPL(zwlr_layer_shell_v1, get_layer_surface) {
    RESOURCE_ARG(wl_surface, surface, 1);
    struct surface_data_t* data = wl_resource_get_user_data(surface);
    surface_data_set_role(data, SURFACE_ROLE_LAYER);
    wl_resource_set_user_data(new_resource, data);
    data->layer_send_configure = 1;
    data->layer_surface = new_resource;
}

REQUEST_OVERRIDE_IMPL(zwlr_layer_surface_v1, destroy) {
    struct surface_data_t* data = wl_resource_get_user_data(zwlr_layer_surface_v1);
    data->layer_surface = NULL;
    surface_data_unmap(data);
}

REQUEST_OVERRIDE_IMPL(ext_session_lock_manager_v1, lock) {
    if (current_session_lock) {
        ext_session_lock_v1_send_finished(new_resource);
    } else {
        current_session_lock = new_resource;
        ext_session_lock_v1_send_locked(new_resource);
    }
}

REQUEST_OVERRIDE_IMPL(ext_session_lock_v1, destroy) {
    if (ext_session_lock_v1 == current_session_lock) {
        FATAL(".destroy (instead of .unlock_and_destroy) called on active lock");
    }
}

REQUEST_OVERRIDE_IMPL(ext_session_lock_v1, unlock_and_destroy) {
    if (ext_session_lock_v1 != current_session_lock) {
        FATAL(".unlock_and_destroy (instead of .destroy) called on inactive lock");
    }
    current_session_lock = NULL;
}

static void lock_surface_send_configure(struct surface_data_t* data, uint32_t width, uint32_t height) {
    data->lock_surface_pending_serial = wl_display_next_serial(display);
    ext_session_lock_surface_v1_send_configure(data->lock_surface, data->lock_surface_pending_serial, width, height);
}

REQUEST_OVERRIDE_IMPL(ext_session_lock_v1, get_lock_surface) {
    RESOURCE_ARG(wl_surface, surface, 1);
    RESOURCE_ARG(wl_output, output, 2);
    ASSERT_EQ(output, output_global, "%p");
    struct surface_data_t* data = wl_resource_get_user_data(surface);
    surface_data_set_role(data, SURFACE_ROLE_SESSION_LOCK);
    wl_resource_set_user_data(new_resource, data);
    data->lock_surface = new_resource;
    lock_surface_send_configure(data, DEFAULT_OUTPUT_WIDTH, DEFAULT_OUTPUT_HEIGHT);
}

REQUEST_OVERRIDE_IMPL(ext_session_lock_surface_v1, ack_configure) {
    UINT_ARG(serial, 0);
    struct surface_data_t* data = wl_resource_get_user_data(ext_session_lock_surface_v1);
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
