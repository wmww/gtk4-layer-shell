#pragma once

struct zwlr_layer_shell_v1;
struct wl_display;

struct zwlr_layer_shell_v1* get_layer_shell_global_from_display(struct wl_display* display);
