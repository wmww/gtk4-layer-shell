#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_widget_set_size_request(GTK_WIDGET(window), 300, 200);
    gtk_layer_init_for_window(window);

    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    ASSERT(!gtk_layer_auto_exclusive_zone_is_enabled(window));
    gtk_layer_auto_exclusive_zone_enable(window);
    ASSERT(gtk_layer_auto_exclusive_zone_is_enabled(window));
    gtk_window_present(window);
}

static void callback_1()
{
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), 200, "%d");
    gtk_widget_set_size_request(GTK_WIDGET(window), 320, 240);
    // TODO
    //gtk_window_resize(window, 1, 1);
}

static void callback_2()
{
    ASSERT_EQ(gtk_layer_get_exclusive_zone(window), 240, "%d");

    ASSERT(gtk_layer_auto_exclusive_zone_is_enabled(window));
    gtk_layer_set_exclusive_zone(window, 20);
    ASSERT(!gtk_layer_auto_exclusive_zone_is_enabled(window));
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
