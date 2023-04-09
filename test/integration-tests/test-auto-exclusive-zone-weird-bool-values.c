#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 300);

    window = create_default_window();

    gtk_layer_init_for_window(window);
    gtk_layer_auto_exclusive_zone_enable(window);

    // Note that true bools are normally 1, but can be other non-zero values
    // This used to cause a problem, as noted in https://github.com/wmww/gtk-layer-shell/pull/79
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, 1);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, 2);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, 3);

    gtk_window_set_default_size(window, 300, 200);
    gtk_window_present(window);
}

TEST_CALLBACKS(
    callback_0,
)
