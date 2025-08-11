#include "integration-test-common.h"

static GtkWindow* window;
static GtkWidget* child;

static void callback_0() {
    EXPECT_MESSAGE(.create_buffer 850 940);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 850 940);

    window = create_default_window();
    child = gtk_window_get_child(window);
    gtk_widget_set_size_request(child, 850, 940);

    gtk_layer_init_for_window(window);
    gtk_window_present(window);
}

static void callback_1() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 850, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 940, "%d");

    EXPECT_MESSAGE(.create_buffer 1001 610);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 1001 610);

    gtk_widget_set_size_request(child, 1001, 610);
}

static void callback_2() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 1001, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 610, "%d");

    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 1001 0);
    EXPECT_MESSAGE(.create_buffer 1001 1080); // size must match DEFAULT_OUTPUT_HEIGHT in common.h

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
}

static void callback_3() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 1001, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 1080, "%d");

    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 0 0);
    EXPECT_MESSAGE(.create_buffer 1920 1080);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
}

static void callback_4() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 1920, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 1080, "%d");

    EXPECT_MESSAGE(.create_buffer 555 777);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_size 555 777);

    gtk_widget_set_size_request(child, 555, 777);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_TOP, FALSE);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
    callback_4,
)
