#pragma once

#include <wayland-client-core.h>

// wayland-private.h
struct wl_object {
	const struct wl_interface *interface;
	const void *implementation;
	uint32_t id;
};

// wayland-client.c
struct wl_proxy {
	struct wl_object object;
	struct wl_display *display;
	struct wl_event_queue *queue;
	uint32_t flags;
	int refcount;
	void *user_data;
	wl_dispatcher_func_t dispatcher;
	uint32_t version;
	const char * const *tag;
	struct wl_list queue_link; // appears in wayland 1.22
};

// wayland-private.h
struct argument_details {
	char type;
	int nullable;
};

// connection.c
static const char *
get_next_argument(const char *signature, struct argument_details *details)
{
	details->nullable = 0;
	for(; *signature; ++signature) {
		switch(*signature) {
		case 'i':
		case 'u':
		case 'f':
		case 's':
		case 'o':
		case 'n':
		case 'a':
		case 'h':
			details->type = *signature;
			return signature + 1;
		case '?':
			details->nullable = 1;
		}
	}
	details->type = '\0';
	return signature;
}
