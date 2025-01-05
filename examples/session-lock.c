#include "gtk4-layer-shell.h"
#include <gtk/gtk.h>

typedef struct {
    GtkApplication *app;
    GtkLayerShellSessionLock *session_lock;
} AppData;

static void
unlock (GtkButton *button, void *data)
{
    (void)button;
    AppData *app_data = (AppData *)data;

    gtk_layer_session_lock_unlock_and_destroy (app_data->session_lock);
    gdk_display_sync (gdk_display_get_default ());

    g_application_quit (G_APPLICATION (app_data->app));
}

static void
locked (GtkLayerShellSessionLock *session_lock, void *data)
{
    (void)data;
    (void)session_lock;
    g_print ("Session locked successfully\n");
}

static void
finished (GtkLayerShellSessionLock *session_lock, void *data)
{
    (void)data;
    (void)session_lock;
    g_critical ("Received finish signal, the session could not be locked\n");
}

static void
activate (GtkApplication* app, void *data)
{
    (void)data;
    g_application_hold (G_APPLICATION (app));

    GtkLayerShellSessionLock *session_lock = gtk_layer_session_lock_new ();
    gtk_layer_session_lock_lock (session_lock);

    GdkDisplay *display = gdk_display_get_default ();
    GListModel *monitors = gdk_display_get_monitors (display);
    guint n_monitors = g_list_model_get_n_items (monitors);

    AppData *app_data = g_new0 (AppData, 1);
    app_data->app = app;
    app_data->session_lock = session_lock;

    g_object_set_data_full (G_OBJECT (app),
                            "app-data",
                            app_data,
                            (GDestroyNotify)g_free);

    g_signal_connect (session_lock, "locked", G_CALLBACK (locked), NULL);
    g_signal_connect (session_lock, "finished", G_CALLBACK (finished), NULL);

    for (guint i = 0; i < n_monitors; ++i) {
        GdkMonitor *monitor = g_list_model_get_item (monitors, i);

        GtkWindow *gtk_window = GTK_WINDOW (gtk_application_window_new (app));
        gtk_layer_session_lock_create_surface_for_window (session_lock, gtk_window, monitor);

        GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
        gtk_widget_set_valign (box, GTK_ALIGN_CENTER);
        gtk_box_set_spacing (GTK_BOX (box), 10);

        GtkWidget *label = gtk_label_new ("GTK Session Lock example");
        gtk_box_append (GTK_BOX (box), label);

        GtkWidget *button = gtk_button_new_with_label ("Unlock");
        g_signal_connect (button, "clicked", G_CALLBACK (unlock), app_data);
        gtk_box_append (GTK_BOX (box), button);

        gtk_window_set_child (GTK_WINDOW (gtk_window), box);
        gtk_window_present (gtk_window);
    }
}

int
main (int argc, char **argv)
{
    GtkApplication *app = gtk_application_new ("com.github.wmww.gtk4-layer-shell.session-lock-example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    int status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    return status;
}
