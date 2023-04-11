# GTK4 Layer Shell Linking Weirdness

## TL;DR
The dynamic linker needs to load `libgtk4-layer-shell.so` before `libwayland-client.so`.

## My code doesn't work, help!
### C/C++ (and other compiled languages)
In your build system, make sure `gtk4-layer-shell` is explicitly linked to and first in the list. For example in meson it should appear first in the list of `dependencies` to your `executable`. To debug where a problem might be follow these steps:
1. Inspect the compiler commands (for example by running `ninja -C build -v`) and make sure gtk4-layer-shell is being linked before libwayland-client (ideally before GTK as well, though that doesn't seem to be necessary)
2. Run `ldd` on your binary and make sure gtk4-layer-shell appears before libwayland
3. Run your binary with `LD_DEBUG=libs` and make sure gtk4-layer-shell is loaded before libwayland. It may be helpful to redirect stderr to stdout and grep for "find library", so a full command might look like `LD_DEBUG=libs build/my-bin 2>&1 | grep 'find library='`

### Python
GI Repository likes to load all dependencies before loading a library, so you need to make sure this is at the top of your main script:
```python
# For GTK4 Layer Shell to get linked before libwayland-client we must explicitly load it before importing with gi
from ctypes import CDLL
CDLL('libgtk4-layer-shell.so')
```
If that still isn't working you can debug with `LD_DEBUG` as described above.

## LD_PRELOAD
You should be able to get most things working using the `LD_PRELOAD` environment variable. Run the program with it set to the path of gtk4-layer-shell:
```
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libgtk4-layer-shell.so my-app
```

## Underlying cause
At time of writing GTK4 has no official support for non-XDG Wayland shells (which is fair enough, since GTK3 only had partial broken support). To get around this we shim some libwayland calls. We define some symbols normally defined by libwayland, so that GTK calls into us when it thinks it's calling into libwayland. We `dlopen` the real libwayland and pass most calls directly on to it, while modifying calls that pertain to XDG Shell and Layer Shell. This allows us to provide GTK the XDG Shell interface it expects and speak Layer Shell to libwayland-client (and beyond that the Wayland compositor).

The main downside is that if libwayland gets loaded before us, it's calls take precedence and the shim doesn't work. We attempt to detect this and display a warning.

## Triggering the error
Triggering the link error intentionally (for example for testing that the library detects it) can be difficult. You need at least the following:
1. Build separate from the gtk4-layer-shell project. If your apps is built within the project (even if it's a different executable) meson links our library first.
2. Link libwayland-client before gtk4-layer-shell
3. Make at least one call to libwayland in your app (otherwise your linking of libwayland-client is ignored)

## Wayland Debug
Old versions of my [wayland-debug](https://github.com/wmww/wayland-debug) tool set used `LD_PRELOAD` to load a patched libwayland, which breaks this library. wayland-debug has been updated to use `LD_LIBRARY_PATH` instead which seems to work fine. If you have any problems please let me know.
