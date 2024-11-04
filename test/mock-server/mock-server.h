// This is an implementation of a mock Wayland compositor for testing
// It does not show anything on the screen, and is only as conforment as is required by GTK

// Useful regex for importing a Wayland protocol. First, copy from header file then:
// Find: (struct (\w+)_interface (\{?\n\t.*)*)[\{;]\n(\t[/ ]\*.*\n)+\tvoid \(\*(\w+)\)(\((.*(,\n)?)*\));\n\};((([^=]*\n)*)\nstatic const struct \w+ \w+ = \{\n((    \.\w+ = \w+\,\n)*)\};)?
// Replace with: \1;\n};\nstatic void \2_\5\6\n{\n    FATAL_NOT_IMPL;\n}\n\{10}\nstatic const struct \2_interface \2_impl = {\n    .\5 = \2_\5,\n\{12}};
// Then remove the struct artifact and replace this with a space: \n\t\s*

#ifndef MOCK_SERVER_H
#define MOCK_SERVER_H

#include "test-common.h"
#include <wayland-server.h>
#include "xdg-shell-server.h"
#include "xdg-dialog-v1-server.h"
#include "wlr-layer-shell-unstable-v1-server.h"

extern struct wl_display* display;

#define ALLOC_STRUCT(type) ((type*)alloc_zeroed(sizeof(type)))
void* alloc_zeroed(size_t size);

#define OVERRIDE_REQUEST(type, method) install_request_override(&type##_interface, #method, type##_##method)
#define NEW_ID_ARG(name, index) ASSERT(type_code_at_index(message, index) == 'n'); uint32_t name = args[index].n;
#define RESOURCE_ARG(type, name, index) ASSERT(type_code_at_index(message, index) == 'o'); ASSERT(message->types[index] == &type##_interface); struct wl_resource* name = (struct wl_resource*)args[index].o;
#define UINT_ARG(name, index) ASSERT(type_code_at_index(message, index) == 'u'); uint32_t name = args[index].u;

typedef void (*RequestOverrideFunction)(struct wl_resource* resource, const struct wl_message* message, union wl_argument* args);
void install_request_override(const struct wl_interface* interface, const char* name, RequestOverrideFunction function);
void use_default_impl(struct wl_resource* resource);
void default_global_create(struct wl_display* display, const struct wl_interface* interface, int version);
char type_code_at_index(const struct wl_message* message, int index);

void init();

#endif // MOCK_SERVER_H
