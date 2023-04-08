#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_keyboard_interactivity 1);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .set_keyboard_interactivity 0);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_keyboard_interactivity(window, TRUE);
    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_layer_set_keyboard_interactivity(window, FALSE);
}

TEST_CALLBACKS(
    callback_0,
)
