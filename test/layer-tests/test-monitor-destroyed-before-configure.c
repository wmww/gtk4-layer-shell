#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .closed);
    UNEXPECT_MESSAGE(zwlr_layer_surface_v1 .configure);

    send_command("enable_configure_delay", "configure_delay_enabled");
    send_command("destroy_outputs_on_layer_surface_create", "destroy_outputs_on_layer_surface_create_enabled");

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_window_present(window);
    ASSERT(gtk_widget_get_mapped(GTK_WIDGET(window)));
}

static void callback_1()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .configure);
    send_command("create_output 1000 1000", "output_created");
}

static void callback_2()
{
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .destroy);
    ASSERT(gtk_widget_get_mapped(GTK_WIDGET(window)));
    gtk_widget_unmap(GTK_WIDGET(window));
    ASSERT(!gtk_widget_get_mapped(GTK_WIDGET(window)));
}

static void callback_3()
{
    UNEXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    send_command("destroy_output 1", "output_destroyed");
    send_command("create_output 1000 1000", "output_created");
    ASSERT(!gtk_widget_get_mapped(GTK_WIDGET(window)));
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
    callback_3,
)
