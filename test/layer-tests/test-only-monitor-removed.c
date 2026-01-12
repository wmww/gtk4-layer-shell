#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);
    window = create_default_window();
    gtk_layer_init_for_window(window);
    for (int i = 0; i < GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER; i++) gtk_layer_set_anchor(window, i, TRUE);
    ASSERT_EQ(g_list_model_get_n_items(gdk_display_get_monitors(gdk_display_get_default())), 1, "%d");
    gtk_window_present(window);
}

static void callback_1() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .closed);
    UNEXPECT_MESSAGE(wl_surface .destroy);
    send_command("destroy_output 0", "output_destroyed");
}

static void callback_2() {
    ASSERT(gtk_widget_get_mapped(GTK_WIDGET(window)));
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .configure 808 911);
    send_command("create_output 808 911", "output_created");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
