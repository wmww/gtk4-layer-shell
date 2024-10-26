project('gtk4-layer-shell',
    ['c'],
    version: '1.0.3',
    license: 'MIT',
    meson_version: '>=0.51.0',
    default_options: ['c_std=gnu11', 'warning_level=3'])

lib_so_version = '0'

add_project_arguments(
    [
        '-Wno-pedantic',
        '-Werror=implicit-function-declaration',
        '-Werror=return-type',
    ],
    language: 'c')

gtk = dependency('gtk4')
wayland_client = dependency('wayland-client', version: '>=1.10.0')

# only required for the tests
wayland_server = dependency('wayland-server', version: '>=1.10.0', required: false)

# wayland_scanner is required, but we can find it without pkg-config
wayland_scanner = dependency('wayland-scanner', version: '>=1.10.0', required: false, native: true)

# required, see https://github.com/wmww/gtk4-layer-shell/issues/24
wayland_protocols = dependency('wayland-protocols', version: '>=1.16', required: true)

pkg_config = import('pkgconfig')
gnome = import('gnome')

subdir('include')
subdir('protocol')
subdir('src')

gtk_layer_shell = declare_dependency(
    link_with: gtk_layer_shell_lib,
    include_directories: gtk_layer_shell_inc)

subdir('examples')

if get_option('docs')
    subdir('doc')
endif

if get_option('tests')
    subdir('test')
else
    # Add a single always-failing test to tell the user to reconfigure with tests enabled
    test('Tests not enabled', find_program('sh'), args: ['-c',
        'echo \'you must run `meson setup --reconfigure -Dtests=true build` in order to run the tests (where build is the path to your build directory)\'; exit 1'])
endif
