#include "integration-test-common.h"

static GtkWindow* window;

static void callback_0() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);

    window = create_default_window();
    gtk_layer_init_for_window(window);
    gtk_layer_set_namespace(window, "remap-before");
    gtk_window_present(window);
}

static void callback_1() {
    EXPECT_MESSAGE(wl_surface .attach nil);
    EXPECT_MESSAGE(wl_surface .commit);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .destroy);
    EXPECT_MESSAGE(wl_surface .destroy);
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface nil);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .configure);
    EXPECT_MESSAGE(zwlr_layer_surface_v1 .ack_configure);

    gtk_layer_set_namespace(window, "remap-after");
    ASSERT_STR_EQ(gtk_layer_get_namespace(window), "remap-after");
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
)
