#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface 2);
    EXPECT_MESSAGE(wl_surface .commit);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_TOP);
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_layer 3);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
}

static void callback_2()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_layer 0);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BACKGROUND);
}

static void callback_3()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_layer 1);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BOTTOM);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
