#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 200);
    EXPECT_MESSAGE(wl_surface .commit);

    window = create_default_window();
    gtk_window_set_default_size(window, 300, 200);
    gtk_layer_init_for_window(window);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_auto_exclusive_zone_enable(window);
    gtk_window_present(window);
}

static void callback_1() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 0);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_layer_set_exclusive_zone(window, 0);
}

static void callback_2() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 20);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_layer_set_exclusive_zone(window, 20);
}

static void callback_3() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_zone 200);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_auto_exclusive_zone_enable(window);
    ASSERT(gtk_layer_auto_exclusive_zone_is_enabled(window));
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), 200, "%d");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
