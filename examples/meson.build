subdir('demo')

executable(
    'gtk4-layer-demo',
    demo_srcs,
    build_by_default: get_option('examples'),
    dependencies: [gtk, gtk_layer_shell],
    install: get_option('examples'))

executable(
    'simple-example-c',
    files('simple-example.c'),
    build_by_default: get_option('examples'),
    dependencies: [gtk, gtk_layer_shell],
    install: false)

executable(
    'session-lock-c',
    files('session-lock.c'),
    build_by_default: get_option('examples'),
    dependencies: [gtk, gtk_layer_shell],
    install: false)

if get_option('vapi')
    add_languages('vala')
    executable(
        'simple-example-vala',
        files('simple-example.vala'),
        dependencies: [gtk, gtk_layer_shell, vapi],
        c_args: [
            '-Wno-unused-but-set-variable',
            '-Wno-unused-parameter',
            '-Wno-cast-function-type',
            '-Wno-deprecated-declarations',
        ],
        install: false)
endif
