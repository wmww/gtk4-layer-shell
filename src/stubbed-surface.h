#pragma once

#include <wayland-client.h>
#include <xdg-shell-client.h>

struct xdg_surface* stubbed_surface_init(struct xdg_wm_base* creating_object, struct wl_surface* surface);
