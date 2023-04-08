#include "integration-test-common.h"

static void callback_0()
{
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_toplevel);
    EXPECT_MESSAGE(xdg_toplevel .configure);
    EXPECT_MESSAGE(xdg_surface .configure);
    EXPECT_MESSAGE(xdg_surface .ack_configure);
    GtkWindow* window = create_default_window();
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .configure);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .ack_configure);
    GtkWindow* window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_window_present(window);
}

static void callback_2()
{
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_toplevel);
    EXPECT_MESSAGE(xdg_toplevel .configure);
    EXPECT_MESSAGE(xdg_surface .configure);
    EXPECT_MESSAGE(xdg_surface .ack_configure);
    GtkWindow* window = create_default_window();
    gtk_window_present(window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
