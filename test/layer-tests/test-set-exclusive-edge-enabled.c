#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_edge 4);
    EXPECT_MESSAGE(wl_surface .commit);

    window = create_default_window();
    gtk_layer_init_for_window(window);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_exclusive_zone(window, 20);
    ASSERT_EQ(gtk_layer_get_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_LEFT), TRUE, "%d");
    ASSERT_EQ(gtk_layer_get_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_RIGHT), TRUE, "%d");
    ASSERT_EQ(gtk_layer_get_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_TOP), TRUE, "%d");
    ASSERT_EQ(gtk_layer_get_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_BOTTOM), TRUE, "%d");
    gtk_window_present(window);
}

static void callback_1() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_edge 0);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
}

static void callback_2() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_edge 1);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
    ASSERT_EQ(gtk_layer_get_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_LEFT), FALSE, "%d");
    ASSERT_EQ(gtk_layer_get_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_RIGHT), TRUE, "%d");
}

static void callback_3() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_edge 0);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
}

static void callback_4() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_exclusive_edge 4);
    EXPECT_MESSAGE(wl_surface .commit);

    gtk_layer_set_exclusive_edge_enabled(window, GTK_LAYER_SHELL_EDGE_TOP, FALSE);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
    callback_4,
)
