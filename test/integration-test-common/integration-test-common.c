#include "integration-test-common.h"
#include <fcntl.h>
#include <sys/stat.h>

int step_time = 300;

static int return_code = 0;
static int callback_index = 0;
static gboolean auto_continue = FALSE;

void send_command(const char* command, const char* expected_response) {
    fprintf(stderr, "sending command: %s\n", command);

    const char* rx_path = getenv("SERVER_TO_CLIENT_FIFO");

    ASSERT(rx_path);
    int rx_fd;
    mkfifo(rx_path, 0666);

    const char* tx_path = getenv("CLIENT_TO_SERVER_FIFO");
    ASSERT(tx_path);
    int tx_fd;
    ASSERT((tx_fd = open(tx_path, O_WRONLY)) >= 0);
    ASSERT(write(tx_fd, command, strlen(command)) > 0);
    ASSERT(write(tx_fd, "\n", 1) > 0);
    close(tx_fd);

    fprintf(stderr, "awaiting response: %s\n", expected_response);
    ASSERT((rx_fd = open(rx_path, O_RDONLY)) >= 0);
#define BUFFER_SIZE 1024
    char buffer[BUFFER_SIZE];
    int length = 0;
    while (TRUE) {
        ASSERT(length < BUFFER_SIZE);
        char* c = buffer + length;
        ssize_t bytes_read = read(rx_fd, c, 1);
        ASSERT(bytes_read > 0);
        if (*c == '\n') {
            *c = '\0';
            ASSERT_STR_EQ(buffer, expected_response);
            break;
        } else {
            length++;
        }
    }
    close(rx_fd);
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

struct lock_signal_data_t {

};

static void on_locked(GtkSessionLockInstance *lock, void *data) {
    (void)lock;
    enum lock_state_t* state = data;
    ASSERT_EQ(*state, LOCK_STATE_UNLOCKED, "%d");
    *state = LOCK_STATE_LOCKED;
}

static void on_failed(GtkSessionLockInstance *lock, void *data) {
    (void)lock;
    enum lock_state_t* state = data;
    ASSERT_EQ(*state, LOCK_STATE_UNLOCKED, "%d");
    *state = LOCK_STATE_FAILED;
}

static void on_unlocked(GtkSessionLockInstance *lock, void *data) {
    (void)lock;
    enum lock_state_t* state = data;
    ASSERT_EQ(*state, LOCK_STATE_LOCKED, "%d");
    *state = LOCK_STATE_UNLOCKED;
}

void connect_lock_signals(GtkSessionLockInstance* lock, enum lock_state_t* state) {
    g_signal_connect(lock, "locked", G_CALLBACK(on_locked), state);
    g_signal_connect(lock, "failed", G_CALLBACK(on_failed), state);
    g_signal_connect(lock, "unlocked", G_CALLBACK(on_unlocked), state);
}

void create_lock_windows(GtkSessionLockInstance* lock, GtkWindow* (*builder)()) {
    GdkDisplay *display = gdk_display_get_default();
    GListModel *monitors = gdk_display_get_monitors(display);
    guint n_monitors = g_list_model_get_n_items(monitors);

    for (guint i = 0; i < n_monitors; ++i) {
        GdkMonitor *monitor = g_list_model_get_item(monitors, i);

        GtkWindow* window = builder();
        gtk_session_lock_instance_assign_window_to_monitor(lock, window, monitor);
        gtk_window_present(window);
    }
}

void create_default_lock_windows(GtkSessionLockInstance* lock) {
    create_lock_windows(lock, create_default_window);
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

    while (g_list_model_get_n_items(gtk_window_get_toplevels()) > 0)
        g_main_context_iteration(NULL, TRUE);

    return return_code;
}
