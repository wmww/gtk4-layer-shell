#include "gtk-layer-demo.h"

const char *current_monitor_key = "current_layer_monitor";

#include "gtk-layer-demo.h"

#define MAX_MONIITORS 100
const char *monitor_strs[MAX_MONIITORS] = {"Default", NULL};
GdkMonitor *monitor_vals[MAX_MONIITORS] = {NULL};

static void
on_monitor_selected (GtkDropDown *dropdown, const GParamSpec *_pspec, GtkWindow *layer_window)
{
    (void)_pspec;
    guint index = gtk_drop_down_get_selected (dropdown);
    gtk_layer_set_monitor (layer_window, monitor_vals[index]);
}

GtkWidget *
monitor_selection_new (GtkWindow *layer_window)
{
    GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    {
        GListModel *monitors = gdk_display_get_monitors (gdk_display_get_default ()); // owned by display
        for (unsigned i = 0; i < g_list_model_get_n_items (monitors) && i < MAX_MONIITORS - 2; i++) {
            GdkMonitor *monitor = g_list_model_get_item (monitors, i);
            GString *text = g_string_new ("");
            g_string_printf (text, "%d. %s", i + 1, gdk_monitor_get_model (monitor));
            monitor_strs[i + 1] = g_strdup (text->str);
            g_string_free (text, TRUE);
            monitor_vals[i + 1] = g_object_ref (monitor);
            monitor_strs[i + 2] = NULL;
        }
        GtkWidget *dropdown = gtk_drop_down_new_from_strings(monitor_strs);
        gtk_widget_set_tooltip_text (dropdown, "Monitor");
        gtk_drop_down_set_selected (GTK_DROP_DOWN (dropdown), 0);
        g_signal_connect (dropdown, "notify::selected", G_CALLBACK (on_monitor_selected), layer_window);
        gtk_box_append (GTK_BOX (vbox), dropdown);
    }
    return vbox;
}
