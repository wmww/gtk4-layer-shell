# Changelog

## [Unreleased]
## [1.1.1] - 29 Mar 2025
- Bump required meson version to 0.54.0
- Control which symbols are exported, do not expose private/unstable functions which have never been in public headers
- Correct annotations in doc comments
- Automatically remap surfaces when monitors change

## [1.1.0] - 29 Jan 2025
This release includes two important new features.

### ext_session_lock_v1
A 2nd Wayland protocol, is used to build lock screens for [these compositors](https://wayland.app/protocols/ext-session-lock-v1#compositor-support).

### liblayer-shell-preload.so
Hack for running arbitrary Wayland windows as Layer Shell surfaces without recompiling. More details [here](https://github.com/wmww/gtk4-layer-shell/blob/main/layer_shell_preload.md).

Changelog:
- Major refactor and improvements to internal architecture, made new features possible but this release is more likely to introduce bugs than others
- Use `RTLD_NEXT` in libwayland-shim, dlopening libwayland is no longer needed
- Add support for `ext_session_lock_v1` protocol (thanks @happenslol for the initial implementation!)
- Add examples, tests and docs for Session Lock support
- Add and document `liblayer-shell-preload.so`

## [1.0.4] - 4 Nov 2024
- Fix protocol error on Wayfire, due to sending zwlr_layer_shell_surface_v1->configure too early
- Fix crash when hovering over a tooltip on Hyprland, due to using an old copy of xdg-shell.xml
- Fix protocol error on GTK4 >=4.16 on KDE and Hyprland, due to request arguments including references to client-only objects
- Add xdg-dialog-v1 to the mock server used by the tests, so the invalid argument bug is regression tested
- Drop Lua example and smoke test (the library should work with Lua just as well as before, but this is no longer tested or officially supported)
- Fix and suppress various warnings

## [1.0.3] - 10 Sep 2024
- Tests: make tests compatible with new libwayland format
- Tests: fix `integration-test-menu-popup` by sending wl_buffer.release in mock server
- Fix: dlopen `libwayland-client.so.0` in addition to `libwayland-client.so`, fixes [#39](https://github.com/wmww/gtk4-layer-shell/issues/39)

## [1.0.2] - 7 Nov 2023
- Fix tests on Arch
- Realize and unrealize on remap instead of setting visibility
- Fix major use-after-free bug causing many crashes

## [1.0.1] - 1 Jul 2023
- Add lua example
- Add links to Rust and Ruby bindings
- Fix doc name conflicts with GTK3 library version
- Add smoke tests

## [1.0.0] - 11 Apr 2023
- Port library and examples from GTK3 to GTK4
- Remove deprecated functions `gtk_layer_set_keyboard_interactivity()` and `gtk_layer_get_keyboard_interactivity()` (`gtk_layer_set_keyboard_mode()` and `gtk_layer_get_keyboard_mode()` can be used instead)
- Change how layer surface window size is controlled, use `gtk_window_set_default_size()` now
- Build [documentation](https://wmww.github.io/gtk4-layer-shell/) with GitHub actions and host with GitHub Pages
- __EDIT:__ Change license from LGPL to MIT (most of the gtk-layer-shell code was always MIT, and the LGPL bits have all been dropped)
