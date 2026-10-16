#include "integration-test-common.h"
#include "dlsym-interposer.h"

// This test is linked against dlsym-interposer, which makes RTLD_NEXT lookups from the shim find the shim itself

static GtkWindow* window;

static void callback_0() {
    window = create_default_window();
    gtk_layer_init_for_window(window);
}

static void callback_1() {
    EXPECT_MESSAGE(zwlr_layer_shell_v1 .get_layer_surface);
    EXPECT_MESSAGE(wl_surface .commit);
    gtk_window_present(window);
}

static void callback_2() {
    // Make sure the interposer was actually in the path of the shim's symbol lookups
    ASSERT(dlsym_interposer_get_call_count() > 0);
    gtk_window_close(window);
}

TEST_CALLBACKS(
    callback_0,
    callback_1,
    callback_2,
)
