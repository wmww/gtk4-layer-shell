#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, 1);
    ASSERT_EQ(gtk_layer_get_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT), 1, "%d");
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, 2);
    ASSERT_EQ(gtk_layer_get_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT), 1, "%d");

    gtk_window_present(window);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, -1);
    ASSERT_EQ(gtk_layer_get_anchor(window, GTK_LAYER_SHELL_EDGE_TOP), 1, "%d");
}

TEST_CALLBACKS(
    callback_0,
)
