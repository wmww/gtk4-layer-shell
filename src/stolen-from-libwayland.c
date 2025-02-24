#include "stolen-from-libwayland.h"

// wayland-private.h
#define WL_CLOSURE_MAX_ARGS 20

// connection.c
static void
wl_argument_from_va_list(const char *signature, union wl_argument *args,
			 int count, va_list ap)
{
	int i;
	const char *sig_iter;
	struct argument_details arg;

	sig_iter = signature;
	for (i = 0; i < count; i++) {
		sig_iter = get_next_argument(sig_iter, &arg);

		switch(arg.type) {
		case 'i':
			args[i].i = va_arg(ap, int32_t);
			break;
		case 'u':
			args[i].u = va_arg(ap, uint32_t);
			break;
		case 'f':
			args[i].f = va_arg(ap, wl_fixed_t);
			break;
		case 's':
			args[i].s = va_arg(ap, const char *);
			break;
		case 'o':
			args[i].o = va_arg(ap, struct wl_object *);
			break;
		case 'n':
			args[i].o = va_arg(ap, struct wl_object *);
			break;
		case 'a':
			args[i].a = va_arg(ap, struct wl_array *);
			break;
		case 'h':
			args[i].h = va_arg(ap, int32_t);
			break;
		case '\0':
			return;
		}
	}
}

// The following functions may be unnecessary, as they are unmodified from libwayland, however it is possible that some
// builds of libwayland could inline the call they make to wl_proxy_marshal_array_flags(), which means we need to
// override all functions that call it.

// wayland-client.c
__attribute__((__visibility__("default")))
struct wl_proxy *
wl_proxy_marshal_flags(struct wl_proxy *proxy, uint32_t opcode,
		       const struct wl_interface *interface,
		       uint32_t version,
		       uint32_t flags, ...)
{
    union wl_argument args[WL_CLOSURE_MAX_ARGS];
	va_list ap;

	va_start(ap, flags);
	wl_argument_from_va_list(proxy->object.interface->methods[opcode].signature,
				 args, WL_CLOSURE_MAX_ARGS, ap);
	va_end(ap);

	return wl_proxy_marshal_array_flags(proxy, opcode, interface, version, flags, args);
}

// wayland-client.c
__attribute__((__visibility__("default")))
void
wl_proxy_marshal(struct wl_proxy *proxy, uint32_t opcode, ...)
{
	union wl_argument args[WL_CLOSURE_MAX_ARGS];
	va_list ap;

	va_start(ap, opcode);
	wl_argument_from_va_list(proxy->object.interface->methods[opcode].signature,
				 args, WL_CLOSURE_MAX_ARGS, ap);
	va_end(ap);

	wl_proxy_marshal_array_constructor(proxy, opcode, args, NULL);
}

// wayland-client.c
__attribute__((__visibility__("default")))
void
wl_proxy_marshal_array(struct wl_proxy *proxy, uint32_t opcode,
		       union wl_argument *args)
{
	wl_proxy_marshal_array_constructor(proxy, opcode, args, NULL);
}

// wayland-client.c
__attribute__((__visibility__("default")))
struct wl_proxy *
wl_proxy_marshal_constructor(struct wl_proxy *proxy,
			     uint32_t opcode,
			     const struct wl_interface *interface,
			     ...)
{
	union wl_argument args[WL_CLOSURE_MAX_ARGS];
	va_list ap;

	va_start(ap, interface);
	wl_argument_from_va_list(proxy->object.interface->methods[opcode].signature,
				 args, WL_CLOSURE_MAX_ARGS, ap);
	va_end(ap);

	return wl_proxy_marshal_array_constructor(proxy, opcode,
						  args, interface);
}

// wayland-client.c
__attribute__((__visibility__("default")))
struct wl_proxy *
wl_proxy_marshal_constructor_versioned(struct wl_proxy *proxy,
				       uint32_t opcode,
				       const struct wl_interface *interface,
				       uint32_t version,
				       ...)
{
	union wl_argument args[WL_CLOSURE_MAX_ARGS];
	va_list ap;

	va_start(ap, version);
	wl_argument_from_va_list(proxy->object.interface->methods[opcode].signature,
				 args, WL_CLOSURE_MAX_ARGS, ap);
	va_end(ap);

	return wl_proxy_marshal_array_constructor_versioned(proxy, opcode,
							    args, interface,
							    version);
}

// wayland-client.c
__attribute__((__visibility__("default")))
struct wl_proxy *
wl_proxy_marshal_array_constructor(struct wl_proxy *proxy,
				   uint32_t opcode, union wl_argument *args,
				   const struct wl_interface *interface)
{
	return wl_proxy_marshal_array_constructor_versioned(proxy, opcode,
							    args, interface,
							    proxy->version);
}

// wayland-client.c
__attribute__((__visibility__("default")))
struct wl_proxy *
wl_proxy_marshal_array_constructor_versioned(struct wl_proxy *proxy,
					     uint32_t opcode,
					     union wl_argument *args,
					     const struct wl_interface *interface,
					     uint32_t version)
{
	return wl_proxy_marshal_array_flags(proxy, opcode, interface, version, 0, args);
}
