#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    send_command("create_output 640 480", "output_created");

    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface wl_output);
    window = create_default_window();
    gtk_layer_init_for_window(window);
    for (int i = 0; i < GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER; i++) gtk_layer_set_anchor(window, i, TRUE);
    ASSERT_EQ(g_list_model_get_n_items(gdk_display_get_monitors(gdk_display_get_default())), 2, "%d");
    GdkMonitor *monitor = g_list_model_get_item(gdk_display_get_monitors(gdk_display_get_default()), 1);
    ASSERT(GDK_IS_MONITOR(monitor));
    gtk_layer_set_monitor(window, monitor);
    gtk_window_present(window);
}

static void callback_1() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);
    send_command("destroy_output 1", "output_destroyed");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
