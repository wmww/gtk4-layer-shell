#include "gtk4-session-lock.h"
#include <gtk/gtk.h>

static GtkApplication* app = NULL;

static void on_unlock_button_clicked(GtkButton *button, void *data) {
    (void)button;
    GtkSessionLockInstance* lock = data;
    gtk_session_lock_instance_unlock(lock);
}

static void on_locked(GtkSessionLockInstance *lock, void *data) {
    (void)lock;
    (void)data;
    g_message("Session locked successfully");
}

static void on_failed(GtkSessionLockInstance *lock, void *data) {
    (void)lock;
    (void)data;
    g_critical("The session could not be locked");
    g_application_quit(G_APPLICATION(app));
}

static void on_unlocked(GtkSessionLockInstance *lock, void *data) {
    (void)lock;
    (void)data;
    g_message("Session unlocked");
}

static void on_monitor(GtkSessionLockInstance* lock, GdkMonitor *monitor, void* data) {
    (void)data;

    GtkWindow *gtk_window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_session_lock_instance_assign_window_to_monitor(lock, gtk_window, monitor);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_box_set_spacing(GTK_BOX(box), 10);

    GtkWidget *label = gtk_label_new("GTK Session Lock example");
    gtk_box_append(GTK_BOX(box), label);

    GtkWidget *button = gtk_button_new_with_label("Unlock");
    g_signal_connect(button, "clicked", G_CALLBACK(on_unlock_button_clicked), lock);
    gtk_box_append(GTK_BOX(box), button);

    // Not displayed, but allows testing that creating popups doesn't crash GTK
    gtk_widget_set_tooltip_text(button, "Foo Bar Baz");

    gtk_window_set_child(GTK_WINDOW(gtk_window), box);
    gtk_window_present(gtk_window);
}

static void lock_display(GtkSessionLockInstance* lock) {
    gtk_session_lock_instance_lock(lock);
}

static void on_lock_button_clicked(GtkWidget* button, void* data) {
    (void)button;
    lock_display(data);
}

static void on_quit_button_clicked(GtkWidget* button, void* data) {
    (void)button;
    GtkSessionLockInstance* lock = data;
    if (gtk_session_lock_instance_is_locked(lock)) {
        g_critical("Quit button somehow pressed while session lock was locked??");
        // If we quit now there would be no way for the user to unlock, don't do that
        return;
    }
    g_message("Quitting");
    g_application_quit(G_APPLICATION(app));
}

// Creates a normal GTK window with buttons to allow the user to re-lock the display or exit
static void create_control_window(GtkSessionLockInstance* lock) {
    GtkWidget* lock_button = gtk_button_new_with_label("Lock");
    g_signal_connect(lock_button, "clicked", G_CALLBACK(on_lock_button_clicked), lock);

    GtkWidget* quit_button = gtk_button_new_with_label("Quit");
    g_signal_connect(quit_button, "clicked", G_CALLBACK(on_quit_button_clicked), lock);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_valign(box, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), lock_button);
    gtk_box_append(GTK_BOX(box), quit_button);

    GtkWindow* control_window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_window_set_child(control_window, box);
    gtk_window_present(control_window);
}

static void activate(GtkApplication* app, void *data) {
    (void)data;
    g_application_hold(G_APPLICATION(app));

    // This creates the lock instance, but does not lock the display yet
    GtkSessionLockInstance* lock = gtk_session_lock_instance_new();
    g_signal_connect(lock, "locked", G_CALLBACK(on_locked), lock);
    g_signal_connect(lock, "failed", G_CALLBACK(on_failed), lock);
    g_signal_connect(lock, "unlocked", G_CALLBACK(on_unlocked), lock);
    g_signal_connect(lock, "monitor", G_CALLBACK(on_monitor), lock);

    create_control_window(lock); // Won't work if called while display is locked
    lock_display(lock);
}

int main(int argc, char **argv) {
    if (!gtk_session_lock_is_supported()) {
        g_message("Session lock not supported");
        return 1;
    }
    app = gtk_application_new(
        "com.github.wmww.gtk4-layer-shell.session-lock-example",
        G_APPLICATION_DEFAULT_FLAGS
    );
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
