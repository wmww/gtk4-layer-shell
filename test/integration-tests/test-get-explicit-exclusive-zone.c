#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), 0, "%d");
    gtk_layer_set_exclusive_zone(window, 12);
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), 12, "%d");
    gtk_window_present(window);
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), 12, "%d");
    gtk_layer_set_exclusive_zone(window, 0);
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), 0, "%d");
    gtk_layer_set_exclusive_zone(window, -1);
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), -1, "%d");
}

TEST_CALLBACKS(
    callback_0,
)
