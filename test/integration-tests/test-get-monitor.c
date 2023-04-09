#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    ASSERT_EQ(g_list_model_get_n_items(gdk_display_get_monitors(gdk_display_get_default())), 1, "%d");
    GdkMonitor *monitor = g_list_model_get_item(gdk_display_get_monitors(gdk_display_get_default()), 0);
    ASSERT(GDK_IS_MONITOR(monitor));
    ASSERT_EQ(gtk_layer_get_monitor(window), NULL, "%p");
    gtk_layer_set_monitor(window, monitor);
    ASSERT_EQ(gtk_layer_get_monitor(window), monitor, "%p");
    gtk_window_present(window);
    ASSERT_EQ(gtk_layer_get_monitor(window), monitor, "%p");
    gtk_layer_set_monitor(window, NULL);
    ASSERT_EQ(gtk_layer_get_monitor(window), NULL, "%p");
}

TEST_CALLBACKS(
    callback_0,
)
