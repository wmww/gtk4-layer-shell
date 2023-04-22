#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_window_present(window);
    const char *name_space = gtk_layer_get_namespace(window);
    ASSERT_STR_EQ(name_space, "gtk4-layer-shell");
}

TEST_CALLBACKS(
    callback_0,
)
