#include "gtk-layer-demo.h"

const char* keyboard_strs[] = {"None", "Exclusive", "On demand", NULL};
GtkLayerShellKeyboardMode keyboard_vals[] = {
    GTK_LAYER_SHELL_KEYBOARD_MODE_NONE,
    GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE,
    GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND,
};

static void on_kb_selected(GtkDropDown* dropdown, const GParamSpec* _pspec, GtkWindow* layer_window) {
    (void)_pspec;
    guint index = gtk_drop_down_get_selected(dropdown);
    gtk_layer_set_keyboard_mode(layer_window, keyboard_vals[index]);
}

GtkWidget* keyboard_selection_new(GtkWindow* layer_window, GtkLayerShellKeyboardMode default_kb) {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6); {
        GtkWidget* label = gtk_label_new("Keyboard:");
        GtkWidget* dropdown = gtk_drop_down_new_from_strings(keyboard_strs);
        gtk_widget_set_tooltip_text(dropdown, "Keyboard interactivity mode");
        for (unsigned i = 0; i < sizeof(keyboard_vals) / sizeof(keyboard_vals[0]); i++) {
            if (keyboard_vals[i] == default_kb)
                gtk_drop_down_set_selected(GTK_DROP_DOWN(dropdown), i);
        }
        g_signal_connect(dropdown, "notify::selected", G_CALLBACK(on_kb_selected), layer_window);
        gtk_box_append(GTK_BOX(vbox), label);
        gtk_box_append(GTK_BOX(vbox), dropdown);
    }

    return vbox;
}
