#include "integration-test-common.h"

static void callback_0()
{
    ASSERT(gtk_layer_is_supported());
    ASSERT(gtk_layer_is_supported());
}

TEST_CALLBACKS(
    callback_0,
)
