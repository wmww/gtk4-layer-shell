#pragma once

#include "xdg-shell-client.h"
#include "wlr-layer-shell-unstable-v1-client.h"
#include "gtk4-layer-shell.h"
#include <gdk/gdk.h>
#include <gdk/gdk.h>

void gtk_wayland_init_if_needed();
struct zwlr_layer_shell_v1* gtk_wayland_get_layer_shell_global();
