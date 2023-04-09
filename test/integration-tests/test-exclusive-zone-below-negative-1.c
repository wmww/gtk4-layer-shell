#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone -1);
    EXPECT_MESSAGE(wl_surface .commit);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_window_present(window);
    gtk_layer_set_exclusive_zone(window, -2);
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), -1, "%d");
}

TEST_CALLBACKS(
    callback_0,
)
