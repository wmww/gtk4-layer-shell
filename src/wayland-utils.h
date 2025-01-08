#pragma once

#include "xdg-shell-client.h"
#include "wlr-layer-shell-unstable-v1-client.h"
#include "gtk4-layer-shell.h"
#include <gdk/gdk.h>
#include <gdk/gdk.h>

void gtk_wayland_init_if_needed();
struct zwlr_layer_shell_v1* gtk_wayland_get_layer_shell_global();

enum xdg_positioner_gravity gdk_gravity_get_xdg_positioner_gravity(GdkGravity gravity);
enum xdg_positioner_anchor gdk_gravity_get_xdg_positioner_anchor(GdkGravity anchor);
enum xdg_positioner_constraint_adjustment gdk_anchor_hints_get_xdg_positioner_constraint_adjustment(GdkAnchorHints hints);
