#include "gtk4-layer-shell.h"
#include "test-common.h"

int main(int argc, char** argv)
{
    ASSERT_EQ(argc, 2, "%d");

    char version_provided_by_gtk_layer_shell[1024];
    sprintf(
        version_provided_by_gtk_layer_shell,
        "%d.%d.%d",
        gtk_layer_get_major_version(),
        gtk_layer_get_minor_version(),
        gtk_layer_get_micro_version());

    ASSERT_STR_EQ(version_provided_by_gtk_layer_shell, argv[1]);

    return 0;
}
