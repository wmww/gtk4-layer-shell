#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    send_command("enable_configure_delay", "configure_delay_enabled");
}

static void callback_1() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface 2);
    EXPECT_MESSAGE(wl_surface .commit);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_window_present(window);
}

static void callback_2() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_layer 3);

    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
