#include "gtk4-session-lock.h"
#include <gtk/gtk.h>

static void unlock(GtkButton *button, void *data) {
    (void)button;
    GtkApplication* app = data;

    gtk_session_lock_unlock();
    g_application_quit(G_APPLICATION(app));
}

static void locked(GtkSessionLockSingleton *session_lock, void *data) {
    (void)data;
    (void)session_lock;
    g_message("Session locked successfully\n");
}

static void finished(GtkSessionLockSingleton *session_lock, void *data) {
    (void)data;
    (void)session_lock;
    g_critical("Received finish signal, the session could not be locked\n");
}

static void activate(GtkApplication* app, void *data) {
    (void)data;
    g_application_hold(G_APPLICATION(app));

    GdkDisplay *display = gdk_display_get_default();
    GListModel *monitors = gdk_display_get_monitors(display);
    guint n_monitors = g_list_model_get_n_items(monitors);

    g_signal_connect(gtk_session_lock_get_singleton(), "locked", G_CALLBACK(locked), NULL);
    g_signal_connect(gtk_session_lock_get_singleton(), "finished", G_CALLBACK(finished), NULL);

    gtk_session_lock_lock();

    for (guint i = 0; i < n_monitors; ++i) {
        GdkMonitor *monitor = g_list_model_get_item(monitors, i);

        GtkWindow *gtk_window = GTK_WINDOW(gtk_application_window_new(app));
        gtk_session_lock_assign_window_to_monitor(gtk_window, monitor);

        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
        gtk_box_set_spacing(GTK_BOX (box), 10);

        GtkWidget *label = gtk_label_new("GTK Session Lock example");
        gtk_box_append(GTK_BOX(box), label);

        GtkWidget *button = gtk_button_new_with_label("Unlock");
        g_signal_connect(button, "clicked", G_CALLBACK(unlock), app);
        gtk_box_append(GTK_BOX(box), button);

        gtk_window_set_child(GTK_WINDOW(gtk_window), box);
        gtk_window_present(gtk_window);
    }
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new(
        "com.github.wmww.gtk4-layer-shell.session-lock-example",
        G_APPLICATION_DEFAULT_FLAGS
    );
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
