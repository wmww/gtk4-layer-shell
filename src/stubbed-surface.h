#pragma once

#include <wayland-client.h>
#include <xdg-shell-client.h>

struct wl_proxy* stubbed_surface_init(struct xdg_wm_base* creating_object, struct wl_surface* surface);
void stubbed_surface_install_positioner_hook();
