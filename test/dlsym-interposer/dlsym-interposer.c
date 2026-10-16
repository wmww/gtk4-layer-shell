// Interposes dlsym() the way some profilers, tracers and security agents do. Since glibc resolves RTLD_NEXT relative to
// whoever calls the real dlsym(), lookups made through this wrapper start after this library instead of after the
// caller. The test that links this library links it before gtk4-layer-shell, so a RTLD_NEXT lookup from the shim would
// find the shim's own symbols (see https://github.com/wmww/gtk4-layer-shell/issues/131).

#define _GNU_SOURCE
#include "dlsym-interposer.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

static int call_count = 0;

__attribute__((__visibility__("default")))
void* dlsym(void* handle, const char* symbol) {
    static void* (*real_dlsym)(void*, const char*) = NULL;
    if (!real_dlsym) {
        real_dlsym = dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.34");
        if (!real_dlsym) {
            fprintf(stderr, "dlsym-interposer: failed to find the real dlsym\n");
            abort();
        }
    }
    call_count++;
    return real_dlsym(handle, symbol);
}

__attribute__((__visibility__("default")))
int dlsym_interposer_get_call_count() {
    return call_count;
}
