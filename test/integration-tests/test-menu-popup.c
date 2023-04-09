#include "integration-test-common.h"

static GtkWindow *window;
static GtkWidget *dropdown;
static const char *options[] = {"Foo", "Bar", "Baz", NULL};

static void callback_0()
{
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);

    // The popup is weirdly slow to open, so slow the tests down
    step_time = 600;

    window = GTK_WINDOW(gtk_window_new());
    dropdown = gtk_drop_down_new_from_strings(options);
    gtk_window_set_child(window, dropdown);
    gtk_layer_init_for_window(window);
    gtk_window_present(window);
}

static void callback_1()
{
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_popup);
    EXPECT_MESSAGE(xdg_popup .grab);

    g_signal_emit_by_name (dropdown, "activate", NULL);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
