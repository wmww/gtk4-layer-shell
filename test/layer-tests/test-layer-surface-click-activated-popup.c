#include "integration-test-common.h"

static GtkWindow *layer_window;
static GtkWidget *layer_dropdown;
static const char *options[] = {"Foo", "Bar", "Baz", NULL};

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);

    // The popup is weirdly slow to open, so slow the tests down
    step_time = 600;

    layer_window = GTK_WINDOW(gtk_window_new());
    layer_dropdown = gtk_drop_down_new_from_strings(options);
    gtk_window_set_child(layer_window, layer_dropdown);
    gtk_layer_init_for_window(layer_window);
    gtk_window_present(layer_window);
}

static void callback_1() {
    EXPECT_MESSAGE(xdg_wm_base .get_xdg_surface);
    EXPECT_MESSAGE(xdg_surface .get_popup nil);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .get_popup);
    EXPECT_MESSAGE(xdg_popup .grab);

    UNEXPECT_MESSAGE(xdg_popup .destroy);

    send_command("click_latest_surface 10 10", "latest_surface_clicked");
}

static void callback_2() {
    EXPECT_MESSAGE(xdg_popup .destroy);
    EXPECT_MESSAGE(xdg_surface .destroy);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .destroy);

    gtk_window_close(layer_window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
