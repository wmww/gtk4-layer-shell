#ifndef GTK_LAYER_SHELL_H
#define GTK_LAYER_SHELL_H

#include <gtk/gtk.h>

/**
 * SECTION:gtk4-layer-shell
 * @title: GTK4 Layer Shell
 * @short_description: A library to use the Layer Shell Wayland protocol with GTK4
 *
 * # Linking against this library and libwayland
 * If you link against libwayland you must link this library before libwayland. See
 * [linking.md](https://github.com/wmww/gtk4-layer-shell/blob/main/linking.md) for details.
 *
 * # Setting Window Size
 * If you wish to force your layer surface window to be a different size than it
 * is by default:
 * |[<!-- language="C" -->
 *   gtk_window_set_default_size (layer_gtk_window, width, height);
 * ]|
 * If width or height is 0, the default is used for that axis. If the window is
 * anchored to opposite edges of the output (see gtk_layer_set_anchor ()), the
 * size requested here is ignored. If you later wish to use the default window size
 * repeat the call with both width and height as 0.
 */

G_BEGIN_DECLS

/**
 * GtkLayerShellLayer:
 * @GTK_LAYER_SHELL_LAYER_BACKGROUND: The background layer.
 * @GTK_LAYER_SHELL_LAYER_BOTTOM: The bottom layer.
 * @GTK_LAYER_SHELL_LAYER_TOP: The top layer.
 * @GTK_LAYER_SHELL_LAYER_OVERLAY: The overlay layer.
 * @GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER: Should not be used except to get the number of entries. (NOTE: may change in
 * future releases as more entries are added)
 */
typedef enum {
    GTK_LAYER_SHELL_LAYER_BACKGROUND,
    GTK_LAYER_SHELL_LAYER_BOTTOM,
    GTK_LAYER_SHELL_LAYER_TOP,
    GTK_LAYER_SHELL_LAYER_OVERLAY,
    GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER, // Should not be used except to get the number of entries
} GtkLayerShellLayer;

/**
 * GtkLayerShellEdge:
 * @GTK_LAYER_SHELL_EDGE_LEFT: The left edge of the screen.
 * @GTK_LAYER_SHELL_EDGE_RIGHT: The right edge of the screen.
 * @GTK_LAYER_SHELL_EDGE_TOP: The top edge of the screen.
 * @GTK_LAYER_SHELL_EDGE_BOTTOM: The bottom edge of the screen.
 * @GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER: Should not be used except to get the number of entries. (NOTE: may change in
 * future releases as more entries are added)
 */
typedef enum {
    GTK_LAYER_SHELL_EDGE_LEFT = 0,
    GTK_LAYER_SHELL_EDGE_RIGHT,
    GTK_LAYER_SHELL_EDGE_TOP,
    GTK_LAYER_SHELL_EDGE_BOTTOM,
    GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER, // Should not be used except to get the number of entries
} GtkLayerShellEdge;

/**
 * GtkLayerShellKeyboardMode:
 * @GTK_LAYER_SHELL_KEYBOARD_MODE_NONE: This window should not receive keyboard events.
 * @GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE: This window should have exclusive focus if it is on the top or overlay layer.
 * @GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND: The user should be able to focus and unfocues this window in an implementation
 * defined way. Not supported for protocol version < 4.
 * @GTK_LAYER_SHELL_KEYBOARD_MODE_ENTRY_NUMBER: Should not be used except to get the number of entries. (NOTE: may change in
 * future releases as more entries are added)
 */
typedef enum {
    GTK_LAYER_SHELL_KEYBOARD_MODE_NONE = 0,
    GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE = 1,
    GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND = 2,
    GTK_LAYER_SHELL_KEYBOARD_MODE_ENTRY_NUMBER = 3, // Should not be used except to get the number of entries
} GtkLayerShellKeyboardMode;

/**
 * gtk_layer_get_major_version:
 *
 * Returns: the major version number of the GTK Layer Shell library
 */
guint gtk_layer_get_major_version ();

/**
 * gtk_layer_get_minor_version:
 *
 * Returns: the minor version number of the GTK Layer Shell library
 */
guint gtk_layer_get_minor_version ();

/**
 * gtk_layer_get_micro_version:
 *
 * Returns: the micro/patch version number of the GTK Layer Shell library
 */
guint gtk_layer_get_micro_version ();

/**
 * gtk_layer_is_supported:
 *
 * May block for a Wayland roundtrip the first time it's called.
 *
 * Returns: %TRUE if the platform is Wayland and Wayland compositor supports the
 * zwlr_layer_shell_v1 protocol.
 */
gboolean gtk_layer_is_supported ();

/**
 * gtk_layer_get_protocol_version:
 *
 * May block for a Wayland roundtrip the first time it's called.
 *
 * Returns: version of the zwlr_layer_shell_v1 protocol supported by the
 * compositor or 0 if the protocol is not supported.
 */
guint gtk_layer_get_protocol_version ();

/**
 * gtk_layer_init_for_window:
 * @window: A #GtkWindow to be turned into a layer surface.
 *
 * Set the @window up to be a layer surface once it is mapped. this must be called before
 * the @window is realized.
 */
void gtk_layer_init_for_window (GtkWindow *window);

/**
 * gtk_layer_is_layer_window:
 * @window: A #GtkWindow that may or may not have a layer surface.
 *
 * Returns: if @window has been initialized as a layer surface.
 */
gboolean gtk_layer_is_layer_window (GtkWindow *window);

/**
 * gtk_layer_get_zwlr_layer_surface_v1:
 * @window: A layer surface.
 *
 * Returns: The underlying layer surface Wayland object
 */
struct zwlr_layer_surface_v1 *gtk_layer_get_zwlr_layer_surface_v1 (GtkWindow *window);

/**
 * gtk_layer_set_namespace:
 * @window: A layer surface.
 * @name_space: The namespace of this layer surface.
 *
 * Set the "namespace" of the surface.
 *
 * No one is quite sure what this is for, but it probably should be something generic
 * ("panel", "osk", etc). The @name_space string is copied, and caller maintains
 * ownership of original. If the window is currently mapped, it will get remapped so
 * the change can take effect.
 *
 * Default is "gtk4-layer-shell" (which will be used if set to %NULL)
 */
void gtk_layer_set_namespace (GtkWindow *window, char const* name_space);

/**
 * gtk_layer_get_namespace:
 * @window: A layer surface.
 *
 * NOTE: this function does not return ownership of the string. Do not free the returned string.
 * Future calls into the library may invalidate the returned string.
 *
 * Returns: a reference to the namespace property. If namespace is unset, returns the
 * default namespace ("gtk4-layer-shell"). Never returns %NULL.
 */
const char *gtk_layer_get_namespace (GtkWindow *window);

/**
 * gtk_layer_set_layer:
 * @window: A layer surface.
 * @layer: The layer on which this surface appears.
 *
 * Set the "layer" on which the surface appears (controls if it is over top of or below other surfaces). The layer may
 * be changed on-the-fly in the current version of the layer shell protocol, but on compositors that only support an
 * older version the @window is remapped so the change can take effect.
 *
 * Default is %GTK_LAYER_SHELL_LAYER_TOP
 */
void gtk_layer_set_layer (GtkWindow *window, GtkLayerShellLayer layer);

/**
 * gtk_layer_get_layer:
 * @window: A layer surface.
 *
 * Returns: the current layer.
 */
GtkLayerShellLayer gtk_layer_get_layer (GtkWindow *window);

/**
 * gtk_layer_set_monitor:
 * @window: A layer surface.
 * @monitor: The output this layer surface will be placed on (%NULL to let the compositor decide).
 *
 * Set the output for the window to be placed on, or %NULL to let the compositor choose.
 * If the window is currently mapped, it will get remapped so the change can take effect.
 *
 * Default is %NULL
 */
void gtk_layer_set_monitor (GtkWindow *window, GdkMonitor *monitor);

/**
 * gtk_layer_get_monitor:
 * @window: A layer surface.
 *
 * NOTE: To get which monitor the surface is actually on, use
 * gdk_display_get_monitor_at_window().
 *
 * Returns: (transfer none): the monitor this surface will/has requested to be on, can be %NULL.
 */
GdkMonitor *gtk_layer_get_monitor (GtkWindow *window);

/**
 * gtk_layer_set_anchor:
 * @window: A layer surface.
 * @edge: A #GtkLayerShellEdge this layer surface may be anchored to.
 * @anchor_to_edge: Whether or not to anchor this layer surface to @edge.
 *
 * Set whether @window should be anchored to @edge.
 * - If two perpendicular edges are anchored, the surface with be anchored to that corner
 * - If two opposite edges are anchored, the window will be stretched across the screen in that direction
 *
 * Default is %FALSE for each #GtkLayerShellEdge
 */
void gtk_layer_set_anchor (GtkWindow *window, GtkLayerShellEdge edge, gboolean anchor_to_edge);

/**
 * gtk_layer_get_anchor:
 * @window: A layer surface.
 * @edge: the edge to which the surface may or may not be anchored
 *
 * Returns: if this surface is anchored to the given edge.
 */
gboolean gtk_layer_get_anchor (GtkWindow *window, GtkLayerShellEdge edge);

/**
 * gtk_layer_set_margin:
 * @window: A layer surface.
 * @edge: The #GtkLayerShellEdge for which to set the margin.
 * @margin_size: The margin for @edge to be set.
 *
 * Set the margin for a specific @edge of a @window. Effects both surface's distance from
 * the edge and its exclusive zone size (if auto exclusive zone enabled).
 *
 * Default is 0 for each #GtkLayerShellEdge
 */
void gtk_layer_set_margin (GtkWindow *window, GtkLayerShellEdge edge, int margin_size);

/**
 * gtk_layer_get_margin:
 * @window: A layer surface.
 * @edge: the margin edge to get
 *
 * Returns: the size of the margin for the given edge.
 */
int gtk_layer_get_margin (GtkWindow *window, GtkLayerShellEdge edge);

/**
 * gtk_layer_set_exclusive_zone:
 * @window: A layer surface.
 * @exclusive_zone: The size of the exclusive zone.
 *
 * Has no effect unless the surface is anchored to an edge. Requests that the compositor
 * does not place other surfaces within the given exclusive zone of the anchored edge.
 * For example, a panel can request to not be covered by maximized windows. See
 * wlr-layer-shell-unstable-v1.xml for details.
 *
 * Default is 0
 */
void gtk_layer_set_exclusive_zone (GtkWindow *window, int exclusive_zone);

/**
 * gtk_layer_get_exclusive_zone:
 * @window: A layer surface.
 *
 * Returns: the window's exclusive zone (which may have been set manually or automatically)
 */
int gtk_layer_get_exclusive_zone (GtkWindow *window);

/**
 * gtk_layer_auto_exclusive_zone_enable:
 * @window: A layer surface.
 *
 * When auto exclusive zone is enabled, exclusive zone is automatically set to the
 * size of the @window + relevant margin. To disable auto exclusive zone, just set the
 * exclusive zone to 0 or any other fixed value.
 *
 * NOTE: you can control the auto exclusive zone by changing the margin on the non-anchored
 * edge. This behavior is specific to gtk4-layer-shell and not part of the underlying protocol
 */
void gtk_layer_auto_exclusive_zone_enable (GtkWindow *window);

/**
 * gtk_layer_auto_exclusive_zone_is_enabled:
 * @window: A layer surface.
 *
 * Returns: if the surface's exclusive zone is set to change based on the window's size
 */
gboolean gtk_layer_auto_exclusive_zone_is_enabled (GtkWindow *window);

/**
 * gtk_layer_set_keyboard_mode:
 * @window: A layer surface.
 * @mode: The type of keyboard interactivity requested.
 *
 * Sets if/when @window should receive keyboard events from the compositor, see
 * GtkLayerShellKeyboardMode for details.
 *
 * Default is %GTK_LAYER_SHELL_KEYBOARD_MODE_NONE
 */
void gtk_layer_set_keyboard_mode (GtkWindow *window, GtkLayerShellKeyboardMode mode);

/**
 * gtk_layer_get_keyboard_mode:
 * @window: A layer surface.
 *
 * Returns: current keyboard interactivity mode for @window.
 */
GtkLayerShellKeyboardMode gtk_layer_get_keyboard_mode (GtkWindow *window);

G_DECLARE_FINAL_TYPE(GtkLayerShellSessionLock, gtk_layer_session_lock, GTK_LAYER_SESSION_LOCK, SESSION_LOCK, GObject)

GtkLayerShellSessionLock * gtk_layer_session_lock_new ();

void gtk_layer_session_lock_lock (GtkLayerShellSessionLock *self);

void gtk_layer_session_lock_destroy (GtkLayerShellSessionLock *self);

void gtk_layer_session_lock_unlock_and_destroy (GtkLayerShellSessionLock *self);

void gtk_layer_session_lock_create_surface_for_window (GtkLayerShellSessionLock *self, GtkWindow *gtk_window, GdkMonitor *gdk_monitor);

/**
 * gtk_layer_session_lock_is_supported:
 *
 * May block for a Wayland roundtrip the first time it's called.
 *
 * Returns: %TRUE if the platform is Wayland and Wayland compositor supports the
 * ext_session_lock_v1 protocol.
 */
gboolean gtk_layer_session_lock_is_supported ();

G_END_DECLS

#endif // GTK_LAYER_SHELL_H
