#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
    ASSERT_EQ(gtk_layer_get_layer(window), GTK_LAYER_SHELL_LAYER_OVERLAY, "%d");
    gtk_window_present(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);
    ASSERT_EQ(gtk_layer_get_layer(window), GTK_LAYER_SHELL_LAYER_TOP, "%d");
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BACKGROUND);
    ASSERT_EQ(gtk_layer_get_layer(window), GTK_LAYER_SHELL_LAYER_BACKGROUND, "%d");
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BOTTOM);
    ASSERT_EQ(gtk_layer_get_layer(window), GTK_LAYER_SHELL_LAYER_BOTTOM, "%d");
}

TEST_CALLBACKS(
    callback_0,
)
