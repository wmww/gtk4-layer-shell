core_srcs = files(
    'stolen-from-libwayland.c',
    'libwayland-shim.c',
    'xdg-surface-server.c',
    'layer-surface.c',
    'session-lock.c',
    'lock-surface.c',
    'stubbed-surface.c',
    'registry.c')

gtk_srcs = files(
    'gtk4-layer-shell.c',
    'gtk4-session-lock.c')

version_args = [
    '-DGTK_LAYER_SHELL_MAJOR=' + meson.project_version().split('.')[0],
    '-DGTK_LAYER_SHELL_MINOR=' + meson.project_version().split('.')[1],
    '-DGTK_LAYER_SHELL_MICRO=' + meson.project_version().split('.')[2],
]

gtk_layer_shell_lib = library('gtk4-layer-shell',
    gtk_srcs, core_srcs, client_protocol_srcs,
    c_args: version_args,
    include_directories: [gtk_layer_shell_inc],
    dependencies: [gtk, wayland_client],
    version: meson.project_version(),
    soversion: lib_so_version,
    install: true)

layer_shell_preload_lib = library('layer-shell-preload',
    files('layer-shell-preload.c'), core_srcs, client_protocol_srcs,
    include_directories: [gtk_layer_shell_inc],
    dependencies: [wayland_client],
    install: true)

pkg_config_name = 'gtk4-layer-shell-0'

# GObject introspection file used to interface with other languages
if get_option('introspection')
    gir_layer = gnome.generate_gir(
        gtk_layer_shell_lib,
        dependencies: [gtk],
        sources: files('../include/gtk4-layer-shell.h') + gtk_srcs + core_srcs,
        namespace: 'Gtk4LayerShell',
        nsversion: '1.0',
        identifier_prefix: 'GtkLayerShell',
        symbol_prefix: 'gtk_layer',
        export_packages: pkg_config_name,
        includes: [ 'Gtk-4.0' ],
        header: 'gtk4-layer-shell.h',
        install: true)

    gir_lock = gnome.generate_gir(
        gtk_layer_shell_lib,
        dependencies: [gtk],
        sources: files('../include/gtk4-session-lock.h') + gtk_srcs + core_srcs,
        namespace: 'Gtk4SessionLock',
        nsversion: '1.0',
        identifier_prefix: 'GtkSessionLock',
        symbol_prefix: 'gtk_session_lock',
        export_packages: pkg_config_name,
        includes: [ 'Gtk-4.0' ],
        header: 'gtk4-session-lock.h',
        install: true)

    if get_option('vapi')
        vapi = gnome.generate_vapi(
            pkg_config_name,
            sources: [gir_layer[0], gir_lock[0]],
            packages: 'gtk4',
            install: true)
    endif
endif

pkg_config.generate(
    name: 'gtk4-layer-shell',
    version: meson.project_version(),
    libraries: [gtk_layer_shell_lib],
    filebase: pkg_config_name,
    subdirs: 'gtk4-layer-shell',
    description: 'Use the Layer Shell Wayland protocol with GTK4',
    url: 'https://github.com/wmww/gtk4-layer-shell')
