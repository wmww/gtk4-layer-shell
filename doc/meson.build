subdir('xml')

glib_prefix = dependency('glib-2.0').get_variable(pkgconfig: 'prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

gnome.gtkdoc(
    'gtk4-layer-shell',
    main_xml: 'gtk4-layer-shell-docs.sgml',
    src_dir: [
        join_paths(meson.source_root(), 'include'),
        join_paths(meson.source_root(), 'src'),
        join_paths(meson.build_root(), 'include'),
    ],
    dependencies: [gtk_layer_shell, gtk, wayland_client, wayland_scanner],
    fixxref_args: [
        '--html-dir=@0@'.format(docpath),
        '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
        '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
        '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gio')),
        '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gtk4')),
    ],
    scan_args: ['--rebuild-types'],
    install_dir: 'gtk4-layer-shell',
    install: true
)
