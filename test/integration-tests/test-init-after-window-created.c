#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(wl_surface .commit);
    window = create_default_window();
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(xdg_surface .destroy);
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_layer_init_for_window(window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
