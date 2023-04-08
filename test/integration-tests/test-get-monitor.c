#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    ASSERT_EQ(gdk_display_get_n_monitors(gdk_display_get_default()), 1, "%d");
    GdkMonitor *monitor = gdk_display_get_monitor(gdk_display_get_default(), 0);
    ASSERT(GDK_IS_MONITOR(monitor));
    ASSERT_EQ(gtk_layer_get_monitor(window), NULL, "%p");
    gtk_layer_set_monitor(window, monitor);
    ASSERT_EQ(gtk_layer_get_monitor(window), monitor, "%p");
    gtk_widget_show_all(GTK_WIDGET(window));
    ASSERT_EQ(gtk_layer_get_monitor(window), monitor, "%p");
    gtk_layer_set_monitor(window, NULL);
    ASSERT_EQ(gtk_layer_get_monitor(window), NULL, "%p");
}

TEST_CALLBACKS(
    callback_0,
)
