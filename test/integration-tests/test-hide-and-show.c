#include "integration-test-common.h"

GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .destroy);
    gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
}

static void callback_2()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .configure);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .ack_configure);
    gtk_widget_set_visible(GTK_WIDGET(window), TRUE);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
