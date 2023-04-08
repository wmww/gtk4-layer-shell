#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);
    gtk_window_close(window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
