#include "integration-test-common.h"

static GtkWindow* window;
static GtkWindow* subsurface;

static void callback_0()
{
    EXPECT_MESSAGE(wl_subcompositor .get_subsurface);
    EXPECT_MESSAGE(wl_subsurface .set_position -20 30);

    window = create_default_window();
    gtk_layer_init_for_window(window);

    subsurface = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
    gtk_container_add(GTK_CONTAINER(subsurface), gtk_label_new("Subsurface"));
    gtk_window_set_modal(subsurface, TRUE);
    gtk_window_set_transient_for(subsurface, window);
    gtk_window_move(subsurface, -20, 30);

    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_widget_show_all(GTK_WIDGET(subsurface));
}

TEST_CALLBACKS(
    callback_0,
)
