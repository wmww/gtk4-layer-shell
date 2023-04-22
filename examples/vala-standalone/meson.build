project('vala-gtk4-layer-shell-example',
    ['vala', 'c'],
    version: '0.1.0',
    license: 'MIT',
    meson_version: '>=0.45.1')

gtk = dependency('gtk4')
gtk_layer_shell = dependency('gtk4-layer-shell-0')

executable(
    'vala-layer-shell',
    files('main.vala'),
    dependencies: [ gtk, gtk_layer_shell ])
