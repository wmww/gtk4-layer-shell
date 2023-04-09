#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    // First, anchor to bottom
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 200);

    window = create_default_window();

    gtk_layer_init_for_window(window);
    gtk_layer_auto_exclusive_zone_enable(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_window_set_default_size(window, 300, 200);
    gtk_window_present(window);
}

static void callback_1()
{
    // Resize
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 240);

    gtk_window_set_default_size(window, 320, 240);
}

static void callback_2()
{
    // Streatch across left edge
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 320);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
}

static void callback_3()
{
    // Stretch across top
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 240);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
}

static void callback_4()
{
    // Anchor right
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 320);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, FALSE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
    callback_4,
)
