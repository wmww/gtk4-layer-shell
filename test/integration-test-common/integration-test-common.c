#include "integration-test-common.h"

// Time in milliseconds for each callback to run
static int step_time = 300;

static int return_code = 0;
static int callback_index = 0;

static gboolean next_step(gpointer _data)
{
    (void)_data;

    CHECK_EXPECTATIONS();
    if (test_callbacks[callback_index]) {
        test_callbacks[callback_index]();
        callback_index++;
        return TRUE;
    } else {
        gtk_main_quit();
        return FALSE;
    }
}

GtkWindow* create_default_window()
{
    GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    GtkWidget *label = gtk_label_new("");
    gtk_label_set_markup(
        GTK_LABEL(label),
        "<span font_desc=\"20.0\">"
            "Layer shell test"
        "</span>");
    gtk_container_add(GTK_CONTAINER(window), label);
    gtk_container_set_border_width(GTK_CONTAINER(window), 12);
    return window;
}

static void continue_button_callback(GtkWidget *_widget, gpointer _data)
{
    (void)_widget; (void)_data;
    next_step(NULL);
}

static void create_debug_control_window()
{
    // Make a window with a continue button for debugging
    GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_layer_init_for_window(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM, 200);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
    GtkWidget* button = gtk_button_new_with_label("Continue ->");
    g_signal_connect (button, "clicked", G_CALLBACK(continue_button_callback), NULL);
    gtk_container_add(GTK_CONTAINER(window), button);
    gtk_widget_show_all(GTK_WIDGET(window));
    // This will only be called once, so leaking the window is fine
}

int main(int argc, char** argv)
{
    gtk_init(0, NULL);

    if (argc == 1) {
        // Run with a debug mode window that lets the user advance manually
        create_debug_control_window();
        next_step(NULL);
    } else if (argc == 2 && g_strcmp0(argv[1], "--auto") == 0) {
        // Run normally with a timeout
        next_step(NULL);
        g_timeout_add(step_time, next_step, NULL);
    } else {
        g_critical("Invalid arguments to integration test");
        return 1;
    }

    gtk_main();
    return return_code;
}
