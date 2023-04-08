#include "integration-test-common.h"

static GtkWindow *window;

static void callback_0()
{
    // This should fail because the tokens are in the wrong order
    EXPECT_MESSAGE(.get_layer_surface zwlr_layer_shell_v1);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_widget_show_all(GTK_WIDGET(window));
}

TEST_CALLBACKS(
    callback_0,
)
