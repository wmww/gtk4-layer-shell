#include "integration-test-common.h"

static GtkWindow* normal_window;
static GtkWidget* popuper;

static void callback_0() {
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_toplevel);

    UNEXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);

    normal_window = GTK_WINDOW(gtk_window_new());
    popuper = popup_widget_new();
    gtk_window_set_child(normal_window, popuper);
    gtk_window_present(normal_window);
}

static void callback_1() {
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_popup xdg_surface);
    EXPECT_MESSAGE(xdg_popup .grab);

    UNEXPECT_MESSAGE(zwlr_layer_surface_v1 .get_popup);
    UNEXPECT_MESSAGE(xdg_popup .destroy);

    popup_widget_toggle_open(popuper);
}

static void callback_2() {
    // Must be in correct order
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
