#ifndef GTK_SESSION_LOCK_H
#define GTK_SESSION_LOCK_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GtkSessionLockSingleton, gtk_session_lock_singleton, GTK_SESSION_LOCK, SESSION_LOCK, GObject)

/**
 * gtk_session_lock_get_singleton:
 *
 * Returns: (transfer none): The singleton instance (created on first call).
 */
GtkSessionLockSingleton* gtk_session_lock_get_singleton();

void gtk_session_lock_lock();
void gtk_session_lock_unlock();
void gtk_session_lock_assign_window_to_monitor(GtkWindow *window, GdkMonitor *monitor);

G_END_DECLS

#endif // GTK_SESSION_LOCK_H
