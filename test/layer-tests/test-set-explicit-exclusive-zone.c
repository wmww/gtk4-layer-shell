#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    UNEXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone);
    EXPECT_MESSAGE(wl_surface .commit);
    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_window_present(window);
}

static void callback_1() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 20);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_layer_set_exclusive_zone(window, 20);
}

static void callback_2() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 0);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_layer_set_exclusive_zone(window, 0);
}

static void callback_3() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone -1);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_layer_set_exclusive_zone(window, -1);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
