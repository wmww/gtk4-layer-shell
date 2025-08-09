#include "integration-test-common.h"
#include <fcntl.h>
#include <sys/stat.h>

int step_time = 300;

static int return_code = 0;
static int callback_index = 0;
static gboolean auto_continue = FALSE;
static gboolean complete = FALSE;

char command_fifo_path[255] = {0};
char response_fifo_path[255] = {0};
static void init_paths() {
    const char* test_dir = getenv("GTKLS_TEST_DIR");
    if (test_dir) {
        char wayland_display[255] = {0};
        sprintf(wayland_display, "%s/gtkls-test-display", test_dir);
        setenv("WAYLAND_DISPLAY", wayland_display, true);
        setenv("XDG_RUNTIME_DIR", test_dir, true);
    } else {
        test_dir = getenv("XDG_RUNTIME_DIR");
    }
    if (!test_dir || strlen(test_dir) == 0) {
        FATAL_FMT("GTKLS_TEST_DIR or XDG_RUNTIME_DIR must be set");
    }
    ASSERT(strlen(test_dir) < 200);
    sprintf(command_fifo_path, "%s/gtkls-test-command", test_dir);
    sprintf(response_fifo_path, "%s/gtkls-test-response", test_dir);
}

void send_command(const char* command, const char* expected_response) {
    fprintf(stderr, "sending command: %s\n", command);

    ASSERT(strlen(response_fifo_path));
    int response_fd;
    mkfifo(response_fifo_path, 0666);

    ASSERT(strlen(command_fifo_path));
    int command_fd;
    ASSERT((command_fd = open(command_fifo_path, O_WRONLY)) >= 0);
    ASSERT(write(command_fd, command, strlen(command)) > 0);
    ASSERT(write(command_fd, "\n", 1) > 0);
    close(command_fd);

    fprintf(stderr, "awaiting response: %s\n", expected_response);
    ASSERT((response_fd = open(response_fifo_path, O_RDONLY)) >= 0);
#define BUFFER_SIZE 1024
    char buffer[BUFFER_SIZE];
    int length = 0;
    while (TRUE) {
        ASSERT(length < BUFFER_SIZE);
        char* c = buffer + length;
        ssize_t bytes_read = read(response_fd, c, 1);
        ASSERT(bytes_read > 0);
        if (*c == '\n') {
            *c = '\0';
            ASSERT_STR_EQ(buffer, expected_response);
            break;
        } else {
            length++;
        }
    }
    close(response_fd);
#undef BUFFER_SIZE
}

static gboolean next_step(gpointer _data) {
    (void)_data;

    CHECK_EXPECTATIONS();
    if (test_callbacks[callback_index]) {
        test_callbacks[callback_index]();
        callback_index++;
        if (auto_continue)
            g_timeout_add(step_time, next_step, NULL);
    } else {
        while (g_list_model_get_n_items(gtk_window_get_toplevels()) > 0)
            gtk_window_destroy(g_list_model_get_item(gtk_window_get_toplevels(), 0));
        complete = TRUE;
    }
    return FALSE;
}

GtkWindow* create_default_window() {
    GtkWindow* window = GTK_WINDOW(gtk_window_new());
    GtkWidget *label = gtk_label_new("");
    gtk_label_set_markup(
        GTK_LABEL(label),
        "<span font_desc=\"20.0\">"
            "Test window"
        "</span>");
    gtk_window_set_child(window, label);
    return window;
}

struct lock_signal_data_t {};

static void on_locked(GtkSessionLockInstance* lock, void* data) {
    (void)lock;
    enum lock_state_t* state = data;
    ASSERT_EQ(*state, LOCK_STATE_UNLOCKED, "%d");
    *state = LOCK_STATE_LOCKED;
}

static void on_failed(GtkSessionLockInstance* lock, void* data) {
    (void)lock;
    enum lock_state_t* state = data;
    ASSERT_EQ(*state, LOCK_STATE_UNLOCKED, "%d");
    *state = LOCK_STATE_FAILED;
}

static void on_unlocked(GtkSessionLockInstance* lock, void* data) {
    (void)lock;
    enum lock_state_t* state = data;
    ASSERT_EQ(*state, LOCK_STATE_LOCKED, "%d");
    *state = LOCK_STATE_UNLOCKED;
}

static void on_monitor(GtkSessionLockInstance* lock, GdkMonitor* monitor, void* data) {
    (void)data;
    GtkWindow* window = create_default_window();
    gtk_session_lock_instance_assign_window_to_monitor(lock, window, monitor);
}

void connect_lock_signals_except_monitor(GtkSessionLockInstance* lock, enum lock_state_t* state) {
    g_signal_connect(lock, "locked", G_CALLBACK(on_locked), state);
    g_signal_connect(lock, "failed", G_CALLBACK(on_failed), state);
    g_signal_connect(lock, "unlocked", G_CALLBACK(on_unlocked), state);
}

void connect_lock_signals(GtkSessionLockInstance* lock, enum lock_state_t* state) {
    connect_lock_signals_except_monitor(lock, state);
    g_signal_connect(lock, "monitor", G_CALLBACK(on_monitor), NULL);
}

static void continue_button_callback(GtkWidget* _widget, gpointer _data) {
    (void)_widget; (void)_data;
    next_step(NULL);
}

static void create_debug_control_window() {
    // Make a window with a continue button for debugging
    GtkWindow *window = GTK_WINDOW(gtk_window_new());
    gtk_layer_init_for_window(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM, 200);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
    GtkWidget* button = gtk_button_new_with_label("Continue ->");
    g_signal_connect (button, "clicked", G_CALLBACK(continue_button_callback), NULL);
    gtk_window_set_child(window, button);
    gtk_window_present(window);
    // This will only be called once, so leaking the window is fine
}

int main(int argc, char** argv) {
    EXPECT_MESSAGE(wl_display .get_registry);

    init_paths();
    gtk_init();

    if (argc == 1) {
        // Run with a debug mode window that lets the user advance manually
        create_debug_control_window();
    } else if (argc == 2 && g_strcmp0(argv[1], "--auto") == 0) {
        // Run normally with a timeout
        auto_continue = TRUE;
    } else {
        g_critical("Invalid arguments to integration test");
        return 1;
    }

    next_step(NULL);
    while (!complete) g_main_context_iteration(NULL, TRUE);
    wl_display_roundtrip(gdk_wayland_display_get_wl_display(gdk_display_get_default()));

    return return_code;
}
