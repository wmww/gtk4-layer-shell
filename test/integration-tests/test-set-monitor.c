#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface wl_output@);
    window = create_default_window();
    gtk_layer_init_for_window(window);
    ASSERT_EQ(gdk_display_get_n_monitors(gdk_display_get_default()), 1, "%d");
    GdkMonitor *monitor = gdk_display_get_monitor(gdk_display_get_default(), 0);
    ASSERT(GDK_IS_MONITOR(monitor));
    gtk_layer_set_monitor(window, monitor);
    gtk_widget_show_all(GTK_WIDGET(window));
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);
    gtk_layer_set_monitor(window, NULL);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
