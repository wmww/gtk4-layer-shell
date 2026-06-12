#include "integration-test-common.h"

#include "ext-session-lock-v1-client.h"

// Same as test-async-failure, but the lock window is owned by a GtkApplication. The rejection's `finished` event is
// dispatched during the assign/present roundtrip, tearing the window down mid-present; destroying an application
// window must happen while it is still realized on GTK >= 4.22 (see test-can-unlock-application-window.c), so this
// crashed with a use-after-free when the teardown unrealized first.

enum lock_state_t state = 0;
struct ext_session_lock_manager_v1* global;
GtkApplication* app;
GtkSessionLockInstance* lock;

static void wl_registry_handle_global(
    void* _data,
    struct wl_registry* registry,
    uint32_t id,
    const char* interface,
    uint32_t version
) {
    (void)_data;

    if (strcmp(interface, ext_session_lock_manager_v1_interface.name) == 0) {
        global = wl_registry_bind(registry, id, &ext_session_lock_manager_v1_interface, version);
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

static void bind_global(struct wl_display* display) {
    struct wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &wl_registry_listener, NULL);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
}

static void on_monitor(GtkSessionLockInstance* lock_, GdkMonitor* monitor, void* data) {
    (void)data;
    GtkWindow* window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_window_set_child(window, gtk_label_new("locked"));
    gtk_session_lock_instance_assign_window_to_monitor(lock_, window, monitor);
}

static void callback_0() {
    // We lock the display without going through the library in order to simulate a lock being held by another process
    bind_global(gdk_wayland_display_get_wl_display(gdk_display_get_default()));
    ASSERT(global);
    struct ext_session_lock_v1* session_lock = ext_session_lock_manager_v1_lock(global);
    (void)session_lock;
}

static void callback_1() {
    EXPECT_MESSAGE(ext_session_lock_manager_v1 .lock);
    EXPECT_MESSAGE(ext_session_lock_v1 .get_lock_surface);
    EXPECT_MESSAGE(ext_session_lock_v1 .finished);

    app = gtk_application_new("com.github.wmww.gtk4-layer-shell.test", G_APPLICATION_DEFAULT_FLAGS);
    ASSERT(g_application_register(G_APPLICATION(app), NULL, NULL));

    lock = gtk_session_lock_instance_new();
    connect_lock_signals_except_monitor(lock, &state);
    g_signal_connect(lock, "monitor", G_CALLBACK(on_monitor), NULL);

    // This may return true or false depending on if the monitor signal handler roundtrips
    gtk_session_lock_instance_lock(lock);
}

static void callback_2() {
    ASSERT_EQ(state, LOCK_STATE_FAILED, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
