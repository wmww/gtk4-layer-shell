#include "gtk-layer-demo.h"

const char *layer_strs[] = {"Overlay", "Top", "Bottom", "Background", NULL};
GtkLayerShellLayer layer_vals[] = {
    GTK_LAYER_SHELL_LAYER_OVERLAY,
    GTK_LAYER_SHELL_LAYER_TOP,
    GTK_LAYER_SHELL_LAYER_BOTTOM,
    GTK_LAYER_SHELL_LAYER_BACKGROUND};

static void
on_layer_selected(GtkDropDown *dropdown, const GParamSpec *_pspec, GtkWindow *layer_window)
{
    (void)_pspec;
    guint index = gtk_drop_down_get_selected (dropdown);
    gtk_layer_set_layer (layer_window, layer_vals[index]);
}

GtkWidget *
layer_selection_new (GtkWindow *layer_window, GtkLayerShellLayer default_layer)
{
    GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    {
        GtkWidget *dropdown = gtk_drop_down_new_from_strings(layer_strs);
        gtk_widget_set_tooltip_text (dropdown, "Layer");
        for (unsigned i = 0; i < sizeof(layer_vals) / sizeof(layer_vals[0]); i++) {
            if (layer_vals[i] == default_layer)
                gtk_drop_down_set_selected (GTK_DROP_DOWN (dropdown), i);
        }
        g_signal_connect (dropdown, "notify::selected", G_CALLBACK (on_layer_selected), layer_window);
        gtk_box_append (GTK_BOX (vbox), dropdown);
    }
    return vbox;
}
