#include "integration-test-common.h"

static GtkWindow* window_a;
static GtkWindow* window_b;

static void callback_0()
{
    window_a = create_default_window();
    window_b = create_default_window();

    gtk_layer_init_for_window(window_a);

    ASSERT(gtk_layer_is_layer_window(window_a));
    ASSERT(!gtk_layer_is_layer_window(window_b));

    gtk_window_present(window_a);
    gtk_window_present(window_b);
}

static void callback_1()
{
    ASSERT(gtk_layer_is_layer_window(window_a));
    ASSERT(!gtk_layer_is_layer_window(window_b));
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
