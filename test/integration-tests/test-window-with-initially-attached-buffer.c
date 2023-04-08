#include "integration-test-common.h"

static GtkWindow* window;

static void on_realize(GtkWidget *widget, gpointer _data)
{
    (void)_data;
    GdkSurface *gdk_surface = gtk_native_get_surface(GTK_NATIVE(widget));
    // This forces GTK to create a cairo surface for the window, which results in in being attached before our custom
    // surface role objects are created/configured, which causes an error unless we clear the attached buffer
    // ^ at least this was the case on GTK3, haven't looked into this at all on GTK4
    cairo_surface_t *cr = gdk_surface_create_similar_surface (gdk_surface, CAIRO_CONTENT_COLOR_ALPHA, 100, 100);
    cairo_surface_destroy(cr);
}

static void callback_0()
{
    window = create_default_window();
    g_signal_connect(G_OBJECT(window), "realize", G_CALLBACK(on_realize), NULL);
    gtk_layer_init_for_window(window);
    gtk_window_present(window);
}
TEST_CALLBACKS(
    callback_0,
)
