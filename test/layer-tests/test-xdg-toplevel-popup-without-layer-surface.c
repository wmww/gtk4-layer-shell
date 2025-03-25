#include "integration-test-common.h"

static GtkWindow *normal_window;
static GtkWidget *normal_dropdown;
static const char *options[] = {"Foo", "Bar", "Baz", NULL};

static void callback_0() {
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_toplevel);

    UNEXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);

    // The popup is weirdly slow to open, so slow the tests down
    step_time = 1200;

    normal_window = GTK_WINDOW(gtk_window_new());
    normal_dropdown = gtk_drop_down_new_from_strings(options);
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), normal_dropdown, 0, 0, 1, 1);
    gtk_window_set_child(normal_window, grid);
    gtk_window_present(normal_window);
}

static void callback_1() {
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_popup xdg_surface);
    EXPECT_MESSAGE(xdg_popup .grab);

    UNEXPECT_MESSAGE(zwlr_layer_surface_v1 .get_popup);
    UNEXPECT_MESSAGE(xdg_popup .destroy);

    g_signal_emit_by_name(normal_dropdown, "activate", NULL);
}

static void callback_2() {
    EXPECT_MESSAGE(xdg_popup .destroy);
    EXPECT_MESSAGE(xdg_surface .destroy);
    EXPECT_MESSAGE(xdg_toplevel .destroy);
    EXPECT_MESSAGE(xdg_surface .destroy);

    gtk_window_close(normal_window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
