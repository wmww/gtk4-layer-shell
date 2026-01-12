#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .closed);
    window = g_object_ref(create_default_window());
    gtk_layer_init_for_window(window);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_LEFT, 10000);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_window_present(window);
}

static void callback_1() {
    ASSERT_EQ(gtk_widget_get_mapped(GTK_WIDGET(window)), FALSE, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
