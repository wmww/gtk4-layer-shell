# GTK4 Layer Shell

![GTK4 Layer Demo screenshot](https://i.imgur.com/dR8X15i.png)

A library for using the Layer Shell and Session Lock Wayland protocols with [GTK4](https://www.gtk.org/). This Library is compatible with C, C++ and any language that supports GObject introspection files (Python, Vala, etc).

The [Layer Shell](https://wayland.app/protocols/wlr-layer-shell-unstable-v1) protocol allows building desktop shell components such as panels, notifications and wallpapers. It can be used to anchor your windows to a corner or edge of the output, or stretch them across the entire output.

The [Session Lock](https://wayland.app/protocols/ext-session-lock-v1) protocol allows building lock screens.

[Documentation](https://wmww.github.io/gtk4-layer-shell/)

[GTK3 version](https://github.com/wmww/gtk-layer-shell)

## Reporting Bugs
To report a crash or other problem using this library open a new [issue on Github](https://github.com/wmww/gtk4-layer-shell/issues). Try to include a minimum reproducer if possible (ideally in C). **DO NOT REPORT GTK4 LAYER SHELL BUGS TO UPSTREAM GTK**. If your program includes GTK4 Layer Shell in any way and misbehaves, assume it's a GTK4 Layer Shell bug. If you can reproduce the problem without including or linking to the gtk4-layer-shell library **at all** then and only then report it to GTK instead of here.

## Supported Desktops
This library only works on Wayland, and only on Wayland compositors that support the Layer Shell protocol. Layer shell __is supported__ on:
- wlroots based compositors (such as __Sway__)
- Smithay based compositors (such as __COSMIC__)
- __Mir__ based compositors (some may not enable the protocol by default. It can be enabled with `--add-wayland-extension zwlr_layer_shell_v1`)
- __KDE Plasma__ on wayland

Layer shell __is not supported__ on:
- Gnome-on-Wayland
- Any X11 desktop

## Using the Library
### Demo
`gtk4-layer-demo` is built if examples are enabled. It's useful for testing layer shell support in compositors. Its code can be found in [examples/demo/](examples/demo/).

### C/C++
The easiest way to build against GTK Layer Shell is to use the `gtk-layer-shell-0` pkg-config package. Refer to your build system or the pkg-config docs for further instructions. [examples/simple-example.c](examples/simple-example.c) is a minimal complete app written in C. __If you link against libwayland, you must link libwayland after gtk4-layer-shell__. See [linking.md](linking.md) for details.

### Python
[examples/simple-example.py](examples/simple-example.py) contains sample Python code.

### Vala
[examples/simple-example.vala](examples/simple-example.vala) contains a minimal working Vala app.

### Rust
[@pentamassiv](https://github.com/pentamassiv) maintains [safe Rust bindings](https://github.com/pentamassiv/gtk4-layer-shell-gir) and the [crates.io crate](https://crates.io/crates/gtk4-layer-shell/). Rust examples can be found [here](https://github.com/pentamassiv/gtk4-layer-shell-gir/tree/main/gtk4-layer-shell/examples).

### Ruby
[mswiger](https://github.com/mswiger) maintains [Ruby bindings](https://github.com/mswiger/ruby-gtk4-layer-shell) which are published to [RubyGems](https://rubygems.org/gems/gtk4_layer_shell).

## Distro Packages
[![List of distros GTK Layer Shell is packaged for](https://repology.org/badge/vertical-allrepos/gtk4-layer-shell.svg)](https://repology.org/project/gtk4-layer-shell/versions)

## Building From Source
1. Clone this repo
2. Install build dependencies (see below)
3. `$ meson setup -Dexamples=true -Ddocs=true -Dtests=true build` (NOTE: `--prefix=/usr` may be needed on Arch Linux)
4. `$ ninja -C build`
5. `$ sudo ninja -C build install`
6. `$ sudo ldconfig`

### Build Dependencies
* [Meson](https://mesonbuild.com/) (>=0.45.1)
* [Ninja](https://ninja-build.org/) (>=1.8.2)
* [libwayland](https://gitlab.freedesktop.org/wayland/wayland) (>=1.10.0)
* [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols) (>=1.16.0)
* [GTK4](https://www.gtk.org/)
* __If `introspection` enabled:__ [GObject introspection](https://gitlab.gnome.org/GNOME/gobject-introspection/)
* __If `docs` enabled:__ [GTK Doc](https://wiki.gnome.org/DocumentationProject/GtkDoc)
* __If `tests` enabled:__ [Python3](https://www.python.org/)
* __If `vapi` enabled:__ [Vala](https://wiki.gnome.org/Projects/Vala)

To install these dependencies on Ubuntu 18.04 and later:
```
sudo apt install meson ninja-build libwayland-dev libgtk-4-dev gobject-introspection libgirepository1.0-dev gtk-doc-tools python3 valac
```

To install on Arch Linux:
```
pacman -S --needed meson ninja gtk4 wayland wayland-protocols gobject-introspection libgirepository gtk-doc python vala
```

### Meson Options
* `-Dexamples` (default `false`): If to build the example C apps; gtk4-layer-demo is installed if examples are built; The Vala example is never built with the rest of the project
* `-Ddocs` (default `false`): If to generate the docs
* `-Dtests` (default `false`): If to build the tests
* `-Dsmoke-tests` (default: `true`): If to test that all examples can run (disable if you don't want to install the various languages and dependencies required)
* `-Dintrospection` (default: `true`): If to build GObject Introspection data (used for bindings to languages other than C/C++)
* `-Dvapi` (default: `true`): If to build VAPI data and Vala example. The VAPI file allows this library to be used in Vala. Requires `-Dintrospection=true`

### Running the Tests
* `ninja -C build test`
* Or, to run a specific test and print the complete output `meson test -C build --verbose <testname>`
* To watch a specific test run against the currently active Wayland compositor `ninja -C build && ./build/test/<testname> --auto`
* To run the test in interactive mode it's same as above, but without the `--auto` flag
* If you have [wayland-debug](https://github.com/wmww/wayland-debug), `wayland-debug -f 'zwlr_*, xdg_*' -r ./build/test/<testname> --auto` can be helpful for debugging

## Licensing
100% MIT (unlike the GTK3 version of this library which contained GPL code copied from GTK)
