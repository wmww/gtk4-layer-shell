#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 240);

    window = create_default_window();

    gtk_layer_init_for_window(window);
    gtk_layer_auto_exclusive_zone_enable(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_window_set_default_size(window, 320, 240);
    gtk_window_present(window);
}

static void callback_1()
{
    // Bottom margin should have no effect on exclusive zone but top margin should
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 250);

    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_TOP, 10);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM, 5);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
