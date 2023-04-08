#include "integration-test-common.h"

static GtkWindow* window;

static void on_realize(GtkWidget *widget, gpointer _data)
{
    (void)_data;
    GdkWindow *gdk_window = gtk_widget_get_window(widget);
    // This forces GTK to create a cairo surface for the window, which results in in being attached before our custom
    // surface role objects are created/configured, which causes an error unless we clear the attached buffer
    cairo_surface_t *cr = gdk_window_create_similar_surface (gdk_window, CAIRO_CONTENT_COLOR_ALPHA, 100, 100);
    cairo_surface_destroy(cr);
}

static void callback_0()
{
    window = create_default_window();
    g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(on_realize), NULL);
    gtk_layer_init_for_window(window);
    gtk_widget_show_all(GTK_WIDGET(window));
}
TEST_CALLBACKS(
    callback_0,
)
