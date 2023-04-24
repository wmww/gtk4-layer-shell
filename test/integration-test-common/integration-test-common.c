#include "integration-test-common.h"

int step_time = 300;

static int return_code = 0;
static int callback_index = 0;
static gboolean auto_continue = FALSE;

static gboolean next_step(gpointer _data)
{
    (void)_data;

    CHECK_EXPECTATIONS();
    if (test_callbacks[callback_index]) {
        test_callbacks[callback_index]();
        callback_index++;
        if (auto_continue)
            g_timeout_add(step_time, next_step, NULL);
    } else {
        while (g_list_model_get_n_items(gtk_window_get_toplevels()) > 0)
            gtk_window_destroy(g_list_model_get_item(gtk_window_get_toplevels(), 0));
    }
    return FALSE;
}

GtkWindow* create_default_window()
{
    GtkWindow* window = GTK_WINDOW(gtk_window_new());
    GtkWidget *label = gtk_label_new("");
    gtk_label_set_markup(
        GTK_LABEL(label),
        "<span font_desc=\"20.0\">"
            "Layer shell test"
        "</span>");
    gtk_window_set_child(window, label);
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
    GtkWindow *window = GTK_WINDOW(gtk_window_new());
    gtk_layer_init_for_window(window);
    gtk_layer_set_anchor(window, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_margin(window, GTK_LAYER_SHELL_EDGE_BOTTOM, 200);
    gtk_layer_set_layer(window, GTK_LAYER_SHELL_LAYER_OVERLAY);
    GtkWidget* button = gtk_button_new_with_label("Continue ->");
    g_signal_connect (button, "clicked", G_CALLBACK(continue_button_callback), NULL);
    gtk_window_set_child(window, button);
    gtk_window_present(window);
    // This will only be called once, so leaking the window is fine
}

int main(int argc, char** argv)
{
    EXPECT_MESSAGE(wl_display .get_registry);

    gtk_init();

    if (argc == 1) {
        // Run with a debug mode window that lets the user advance manually
        create_debug_control_window();
    } else if (argc == 2 && g_strcmp0(argv[1], "--auto") == 0) {
        // Run normally with a timeout
        auto_continue = TRUE;
    } else {
        g_critical("Invalid arguments to integration test");
        return 1;
    }

    next_step(NULL);

    while (g_list_model_get_n_items(gtk_window_get_toplevels()) > 0)
        g_main_context_iteration(NULL, TRUE);

    return return_code;
}
