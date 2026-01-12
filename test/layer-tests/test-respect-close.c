#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);
    window = g_object_ref(create_default_window());
    gtk_layer_init_for_window(window);
    gtk_layer_set_respect_close(window, TRUE);
    ASSERT_EQ(g_list_model_get_n_items(gdk_display_get_monitors(gdk_display_get_default())), 1, "%d");
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .closed);
    EXPECT_MESSAGE(wl_surface .destroy);
    send_command("destroy_output 0", "output_destroyed");
}

static void callback_2()
{
    // Because we set respect_close to TRUE the window should have been automatically unmapped from the .closed event
    ASSERT(!gtk_widget_get_mapped(GTK_WIDGET(window)));
    ASSERT_EQ(G_OBJECT(window)->ref_count, 1, "%d");
    g_object_unref(window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
