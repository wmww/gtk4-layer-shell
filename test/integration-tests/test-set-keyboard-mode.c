#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_keyboard_interactivity 1);
    EXPECT_MESSAGE(wl_surface .commit);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_keyboard_interactivity 0);
    EXPECT_MESSAGE(wl_surface .commit);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
    gtk_window_present(window);
    gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_keyboard_interactivity 2);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_keyboard_mode(window, GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
