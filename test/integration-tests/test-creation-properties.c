#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BOTTOM);
    gtk_layer_set_namespace(window, "foobar");
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_exclusive_zone(window, 32);
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface 1 "foobar");
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 32);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_window_present(window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
