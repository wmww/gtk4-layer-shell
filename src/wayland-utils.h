#pragma once

struct zwlr_layer_shell_v1;

void gtk_wayland_init_if_needed();
struct zwlr_layer_shell_v1* gtk_wayland_get_layer_shell_global();
