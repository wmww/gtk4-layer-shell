#pragma once

struct zwlr_layer_shell_v1;
struct ext_session_lock_manager_v1;
struct wl_display;

struct zwlr_layer_shell_v1* get_layer_shell_global_from_display(struct wl_display* display);
struct ext_session_lock_manager_v1* get_session_lock_global_from_display(struct wl_display* display);
struct wl_subcompositor* get_subcompositor_global_from_display(struct wl_display* display);
