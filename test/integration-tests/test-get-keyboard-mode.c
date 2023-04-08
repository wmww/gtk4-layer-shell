#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    ASSERT_EQ(gtk_layer_get_keyboard_mode(window), GTK_LAYER_SHELL_KEYBOARD_MODE_NONE, "%d");
    gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
    ASSERT_EQ(gtk_layer_get_keyboard_mode(window), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND, "%d");
    gtk_window_present(window);
    ASSERT_EQ(gtk_layer_get_keyboard_mode(window), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND, "%d");
    gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
    ASSERT_EQ(gtk_layer_get_keyboard_mode(window), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE, "%d");
}

TEST_CALLBACKS(
    callback_0,
)
