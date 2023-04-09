#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();

    gtk_layer_init_for_window(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);

    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_TOP, 2);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_RIGHT, 6);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM, 12);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_LEFT, 14);
    gtk_window_present(window);
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_TOP), 2, "%d");
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_RIGHT), 6, "%d");
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM), 12, "%d");
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_LEFT), 14, "%d");
}

static void callback_1()
{
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_TOP, 5);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_RIGHT, 10);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM, 25);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_LEFT, 30);
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_TOP), 5, "%d");
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_RIGHT), 10, "%d");
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM), 25, "%d");
    ASSERT_EQ(gtk_layer_get_margin(window, GTK_LAYER_SHELL_EDGE_LEFT), 30, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
