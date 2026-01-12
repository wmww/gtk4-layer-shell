#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface wl_output);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    for (int i = 0; i < GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER; i++)
        gtk_layer_set_anchor(window, i, TRUE);
    ASSERT_EQ(g_list_model_get_n_items(gdk_display_get_monitors(gdk_display_get_default())), 1, "%d");
    GdkMonitor *monitor = g_list_model_get_item(gdk_display_get_monitors(gdk_display_get_default()), 0);
    ASSERT(GDK_IS_MONITOR(monitor));
    gtk_layer_set_monitor(window, monitor);
    gtk_window_present(window);
}

static void callback_1() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), DEFAULT_OUTPUT_WIDTH, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), DEFAULT_OUTPUT_HEIGHT, "%d");

    send_command("create_output 640 480", "output_created");
}

static void callback_2() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), DEFAULT_OUTPUT_WIDTH, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), DEFAULT_OUTPUT_HEIGHT, "%d");

    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface wl_output);

    ASSERT_EQ(g_list_model_get_n_items(gdk_display_get_monitors(gdk_display_get_default())), 2, "%d");
    GdkMonitor *monitor = g_list_model_get_item(gdk_display_get_monitors(gdk_display_get_default()), 1);
    ASSERT(GDK_IS_MONITOR(monitor));
    gtk_layer_set_monitor(window, monitor);
}

static void callback_3() {
    ASSERT_EQ(gtk_widget_get_width(GTK_WIDGET(window)), 640, "%d");
    ASSERT_EQ(gtk_widget_get_height(GTK_WIDGET(window)), 480, "%d");

    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);

    gtk_layer_set_monitor(window, NULL);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
