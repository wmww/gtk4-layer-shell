# GTK4 Layer Shell

![GTK4 Layer Demo screenshot](https://i.imgur.com/dR8X15i.png)

A library for using the [Layer Shell](https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/blob/master/unstable/wlr-layer-shell-unstable-v1.xml) Wayland protocol with [GTK4](https://www.gtk.org/). With this library you can build desktop shell components such as panels, notifications and wallpapers. You can use it to anchor your windows to a corner or edge of the output, or stretch them across the entire output. This Library is compatible with C, C++ and any language that supports GObject introspection files (Python, Vala, etc).

[Documentation](https://wmww.github.io/gtk4-layer-shell/)

[GTK3 version](https://github.com/wmww/gtk-layer-shell)

## Supported Desktops
This library only works on Wayland, and only on Wayland compositors that support the Layer Shell protocol. Layer shell __is supported__ on:
- wlroots based compositors (such as __Sway__)
- __KDE Plasma__ on wayland
- __Mir__-based compositors (some may not enable the protocol by default. It can be enabled with `--add-wayland-extension zwlr_layer_shell_v1`)

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
[examples/vala-standalone](examples/vala-standalone) contains a minimal working standalone Vala project, see [the readme](examples/vala-standalone/README.md) for details.

## Distro Packages
[![List of distros GTK Layer Shell is packaged for](https://repology.org/badge/vertical-allrepos/gtk4-layer-shell.svg)](https://repology.org/project/gtk4-layer-shell/versions)

## Building From Source
1. Clone this repo
2. Install build dependencies (see below)
3. `$ meson build -Dexamples=true -Ddocs=true -Dtests=true`
4. `$ ninja -C build`
5. `$ sudo ninja -C build install`
6. `$ sudo ldconfig`

### Build Dependencies
* [Meson](https://mesonbuild.com/) (>=0.45.1)
* [libwayland](https://gitlab.freedesktop.org/wayland/wayland) (>=1.10.0)
* [GTK4](https://www.gtk.org/)
* [GObject introspection](https://gitlab.gnome.org/GNOME/gobject-introspection/)
* [GTK Doc](https://www.gtk.org/gtk-doc/) (only required if docs are enabled)
* [Vala](https://wiki.gnome.org/Projects/Vala) (only required if vapi is enabled)

To install these dependencies on Ubuntu 18.04 and later:
```
sudo apt install meson libwayland-dev libgtk-4-dev gobject-introspection libgirepository1.0-dev gtk-doc-tools valac
```

### Meson Options
* `-Dexamples` (default `false`): If to build the example C apps; gtk4-layer-demo is installed if examples are built; The Vala example is never built with the rest of the project
* `-Ddocs` (default `false`): If to generate the docs
* `-Dtests` (default `false`): If to build the tests
* `-Dintrospection` (default: `true`): If to build GObject Introspection data (used for bindings to languages other than C/C++)
* `-Dvapi` (default: `true`): If to build VAPI data (allows this library to be used in Vala). Requires `-Dintrospection=true`

### Running the Tests
* `ninja -C build test`

## Licensing
100% MIT (unlike the GTK3 version of this library which contained GPL code copied from GTK)
