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
 * # Note on popups
 * Popups (such as menus and tooltips) do not currently display while the screen is locked. Please use alternatives,
 * such as GtkPopover (which is backed by a subsurface instead of a popup).
 *
 * # Note On Linking
 * If you link against libwayland you must link this library before libwayland. See
 * [linking.md](https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md) for details.
 */

G_BEGIN_DECLS

/**
 * gtk_session_lock_is_supported:
 *
 * May block for a Wayland roundtrip the first time it's called.
 *
 * Returns: %TRUE if the platform is Wayland and Wayland compositor supports the
 * Session Lock protocol.
 */
gboolean gtk_session_lock_is_supported();

/**
 * GtkSessionLockInstance:
 *
 * An instance of the object used to control locking the screen.
 * Multiple instances can exist at once, but only one can be locked at a time.
 */
G_DECLARE_FINAL_TYPE(GtkSessionLockInstance, gtk_session_lock_instance, GTK_SESSION_LOCK, INSTANCE, GObject)

/**
 * GtkSessionLockInstance::locked:
 *
 * The ::locked signal is fired when the screen is successfully locked.
 */

/**
 * GtkSessionLockInstance::failed:
 *
 * The ::failed signal is fired when the lock could not be acquired.
 */

/**
 * GtkSessionLockInstance::unlocked:
 *
 * The ::unlocked signal is fired when the session is unlocked, which may have been caused by a call to
 * gtk_session_lock_instance_unlock() or by the compositor.
 */

/**
 * gtk_session_lock_instance_new:
 *
 * Returns: new session lock instance
 */
GtkSessionLockInstance* gtk_session_lock_instance_new();

/**
 * gtk_session_lock_instance_lock:
 * @self: the instance to lock
 *
 * Lock the screen. This should be called before assigning any windows to monitors. If this function fails the ::failed
 * signal is emitted, if it succeeds the ::locked signal is emitted. The ::failed signal may be emitted before the
 * function returns (for example, if another #GtkSessionLockInstance holds a lock) or later (if another process holds a
 * lock). The only case where neither signal is triggered is if the instance is already locked.
 *
 * Returns: false on immediate fail, true if lock acquisition was successfully started
 */
gboolean gtk_session_lock_instance_lock(GtkSessionLockInstance* self);

/**
 * gtk_session_lock_instance_unlock:
 * @self: the instance to unlock
 *
 * If the screen is locked by this instance unlocks it and fires ::unlocked. Otherwise has no effect
 */
void gtk_session_lock_instance_unlock(GtkSessionLockInstance* self);

/**
 * gtk_session_lock_instance_is_locked:
 * @self: the instance
 *
 * Returns if this instance currently holds a lock.
 */
gboolean gtk_session_lock_instance_is_locked(GtkSessionLockInstance* self);

/**
 * gtk_session_lock_instance_assign_window_to_monitor:
 * @self: the instance to use
 * @window: The GTK Window to use as a lock surface
 * @monitor: The monitor to show it on
 *
 * This should be called with a different window once for each monitor immediately after calling
 * gtk_session_lock_lock(). Hiding a window that is active on a monitor or not letting a window be resized by the
 * library may result in a Wayland protocol error.
 */
void gtk_session_lock_instance_assign_window_to_monitor(
    GtkSessionLockInstance* self,
    GtkWindow *window,
    GdkMonitor *monitor
);

G_END_DECLS

#endif // GTK_SESSION_LOCK_H
