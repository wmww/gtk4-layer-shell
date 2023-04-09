#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 0 0);
    EXPECT_MESSAGE(.create_buffer 1920 1080); // size must match DEFAULT_OUTPUT_WIDTH/DEFAULT_OUTPUT_HEIGHT in common.h

    window = create_default_window();

    gtk_layer_init_for_window(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BOTTOM);
    gtk_layer_set_namespace(window, "foobar");
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

    gtk_window_set_default_size(window, 600, 700);
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(.create_buffer 600 1080); // size must match DEFAULT_OUTPUT_HEIGHT in common.h
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 600 0);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
}

static void callback_2()
{
    EXPECT_MESSAGE(.create_buffer 600 700);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 600 700);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
