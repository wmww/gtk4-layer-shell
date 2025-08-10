#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 0 0);
    EXPECT_MESSAGE(.create_buffer 1920 1080); // size must match DEFAULT_OUTPUT_WIDTH/DEFAULT_OUTPUT_HEIGHT in common.h

    window = create_default_window();

    gtk_layer_init_for_window(window);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_BOTTOM);
    gtk_layer_set_namespace(window, "foobar");
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

    gtk_window_set_default_size(window, 600, 700);
    gtk_window_present(window);
}

static void callback_1() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 1920, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 1080, "%d");

    EXPECT_MESSAGE(.create_buffer 600 1080); // size must match DEFAULT_OUTPUT_HEIGHT in common.h
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 600 0);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
}

static void callback_2() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 600, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 1080, "%d");

    EXPECT_MESSAGE(.create_buffer 600 700);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 600 700);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
}

static void callback_3() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 600, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 700, "%d");

    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 0 700);
    EXPECT_MESSAGE(.create_buffer 1920 700);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
}

static void callback_4() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 1920, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 700, "%d");

    EXPECT_MESSAGE(.create_buffer 1920 300);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 0 300);

    gtk_window_set_default_size(window, 200, 300);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
    callback_4,
)
