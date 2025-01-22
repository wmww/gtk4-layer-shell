#ifndef GTK_SESSION_LOCK_H
#define GTK_SESSION_LOCK_H

#include <gtk/gtk.h>

/**
 * SECTION:gtk4-session-lock
 * @title: GTK4 Session Lock
 * @short_description: GTK4 support for the Session Lock Wayland protocol
 *
 * [Session Lock](https://wayland.app/protocols/ext-session-lock-v1)
 * is a Wayland protocol for lock screens. Use it to lock the compositor
 * and display the lock screen. This library and the underlying Wayland
 * protocol do not handle authentication.
 *
 * # Note On Linking
 * If you link against libwayland you must link this library before libwayland. See
 * [linking.md](https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md) for details.
 */

G_BEGIN_DECLS

/**
 * GtkSessionLockSingleton:
 *
 * The singleton object used to register signals relating to the lock screen's state.
 */
G_DECLARE_FINAL_TYPE(GtkSessionLockSingleton, gtk_session_lock_singleton, GTK_SESSION_LOCK, SESSION_LOCK, GObject)

/**
 * GtkSessionLockSingleton::locked:
 *
 * The ::locked signal is fired when the screen is successfully locked.
 */

/**
 * GtkSessionLockSingleton::failed:
 *
 * The ::failed signal is fired when the lock could not be acquired.
 */

/**
 * GtkSessionLockSingleton::unlocked:
 *
 * The ::unlocked signal is fired when the session is unlocked.
 */

/**
 * gtk_session_lock_get_singleton:
 *
 * Returns: (transfer none): The singleton instance (created on first call).
 */
GtkSessionLockSingleton* gtk_session_lock_get_singleton();

/**
 * gtk_session_lock_lock:
 *
 * Lock the screen. This should be called before assigning any windows to monitors.
 */
void gtk_session_lock_lock();

/**
 * gtk_session_lock_unlock:
 *
 * Unlock the screen. Has no effect if called when the screen is not locked.
 */
void gtk_session_lock_unlock();

/**
 * gtk_session_lock_is_locked:
 *
 * Returns if the screen is currently locked by this library in this process.
 */
gboolean gtk_session_lock_is_locked();

/**
 * gtk_session_lock_assign_window_to_monitor:
 * @window: The GTK Window to use as a lock surface
 * @monitor: The monitor to show it on
 *
 * This should be called with a different window once for each monitor immediately after calling
 * gtk_session_lock_lock(). Hiding a window that is active on a monitor or not letting a window be resized by the
 * library may result in a protocol error.
 */
void gtk_session_lock_assign_window_to_monitor(GtkWindow *window, GdkMonitor *monitor);

G_END_DECLS

#endif // GTK_SESSION_LOCK_H
