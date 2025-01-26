#include "layer-surface.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

static bool has_created_layer_surface = false;

static bool get_bool_from_env(const char* env) {
    const char* val = getenv(env);
    if (!val || strcmp(val, "") == 0 || strcmp(val, "0") == 0) {
        return false;
    } else if (strcmp(val, "1") == 0) {
        return true;
    } else {
        fprintf(stderr, "Invalid value for %s (expected '1', '0' or unset)", env);
        exit(1);
    }
}

static int get_int_from_env(const char* env) {
    const char* val = getenv(env);
    return val ? atoi(val) : 0;
}

static const char* get_string_from_env(const char* env, const char* fallback) {
    const char* val = getenv(env);
    if (val && *val) {
        return val;
    } else {
        return fallback;
    }
}

static struct geom_edges_t get_edges_from_env(const char* env) {
    const char* val = getenv(env);
    if (!val || strcmp(val, "0") == 0) {
        return (struct geom_edges_t){0};
    } else {
        struct geom_edges_t ret = {0};
        for (const char* c = val; *c; c++) {
            switch (*c) {
                case 'l': ret.left   = true; break;
                case 'r': ret.right  = true; break;
                case 't': ret.top    = true; break;
                case 'b': ret.bottom = true; break;
                default:
                    fprintf(stderr, "Invalid character '%c' in %s (expected 'l', 'r', 't' and 'b')", *c, env);
                    exit(1);
            }
        }
        return ret;
    }
}

static enum zwlr_layer_shell_v1_layer get_layer_from_env(const char* env) {
    const char* val = getenv(env);
    if (!val || strcmp(val, "") == 0 || strcmp(val, "o") == 0 || strcmp(val, "overlay") == 0) {
        return ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    } else if (strcmp(val, "t") == 0 || strcmp(val, "top") == 0) {
        return ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    } else if (strcmp(val, "b") == 0 || strcmp(val, "bottom") == 0) {
        return ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
    } else if (strcmp(val, "g") == 0 || strcmp(val, "background") == 0) {
        return ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
    } else {
        fprintf(stderr, "Invalid value for %s (expected unset/'o'/'overlay', 't'/'top', 'b'/'bottom', 'g'/'background')", env);
        exit(1);
    }
}

static enum zwlr_layer_surface_v1_keyboard_interactivity get_keyboard_interactivity_from_env(const char* env) {
    const char* val = getenv(env);
    if (!val || strcmp(val, "") == 0 || strcmp(val, "n") == 0 || strcmp(val, "none") == 0) {
        return ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE;
    } else if (strcmp(val, "e") == 0 || strcmp(val, "exclusive") == 0) {
        return ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE;
    } else if (strcmp(val, "o") == 0 || strcmp(val, "on-demand") == 0) {
        return ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND;
    } else {
        fprintf(stderr, "Invalid value for %s (expected unset/'n'/'none', 'e'/'exclusive' or 'o'/'on-demand')", env);
        exit(1);
    }
}

static struct geom_size_t get_preferred_size(struct layer_surface_t* surface) {
    (void)surface;
    return (struct geom_size_t){
        .width = get_int_from_env("LAYER_WIDTH"),
        .height = get_int_from_env("LAYER_HEIGHT"),
    };
}

static struct layer_surface_t* layer_surface_hook_callback_impl(struct wl_surface* wl_surface) {
    (void)wl_surface;
    if (has_created_layer_surface && !get_bool_from_env("LAYER_ALL_SURFACES")) {
        return NULL;
    }
    has_created_layer_surface = true;
    struct layer_surface_t* layer_surface = malloc(sizeof(struct layer_surface_t));
    *layer_surface = layer_surface_make();
    layer_surface->get_preferred_size = get_preferred_size;
    layer_surface_set_name_space(layer_surface, get_string_from_env("LAYER_NAMESPACE", "layer-shell-preload"));
    layer_surface_set_anchor(layer_surface, get_edges_from_env("LAYER_ANCHOR"));
    layer_surface_set_layer(layer_surface, get_layer_from_env("LAYER_LAYER"));
    layer_surface_set_keyboard_mode(layer_surface, get_keyboard_interactivity_from_env("LAYER_KEYBOARD"));
    if (get_bool_from_env("LAYER_EXCLUSIVE")) {
        layer_surface_auto_exclusive_zone_enable(layer_surface);
    }
    return layer_surface;
}

__attribute__((constructor))
static void setup(void) {
    layer_surface_install_hook(layer_surface_hook_callback_impl);
}
