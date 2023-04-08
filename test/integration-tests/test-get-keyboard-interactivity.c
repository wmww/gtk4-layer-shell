#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    ASSERT_EQ(gtk_layer_get_keyboard_interactivity(window), FALSE, "%d");
    gtk_layer_set_keyboard_interactivity(window, TRUE);
    ASSERT_EQ(gtk_layer_get_keyboard_interactivity(window), TRUE, "%d");
    gtk_widget_show_all(GTK_WIDGET(window));
    ASSERT_EQ(gtk_layer_get_keyboard_interactivity(window), TRUE, "%d");
    gtk_layer_set_keyboard_interactivity(window, FALSE);
    ASSERT_EQ(gtk_layer_get_keyboard_interactivity(window), FALSE, "%d");
}

TEST_CALLBACKS(
    callback_0,
)
