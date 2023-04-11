# To run this script without installing the library, set GI_TYPELIB_PATH and LD_LIBRARY_PATH to the build/src directory
# GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src python3 examples/simple-example.py

# For GTK4 Layer Shell to get linked before libwayland-client we must explicitly load it before using gi
from ctypes import CDLL
CDLL('libgtk4-layer-shell.so')

import gi
gi.require_version("Gtk", "4.0")
gi.require_version('Gtk4LayerShell', '1.0')

from gi.repository import Gtk
from gi.repository import Gtk4LayerShell as LayerShell

def on_activate(app):
    window = Gtk.Window(application=app)
    window.set_default_size(400, 70)

    LayerShell.init_for_window(window)
    LayerShell.set_layer(window, LayerShell.Layer.TOP)
    LayerShell.set_anchor(window, LayerShell.Edge.BOTTOM, True)
    LayerShell.set_margin(window, LayerShell.Edge.BOTTOM, 20)
    LayerShell.set_margin(window, LayerShell.Edge.TOP, 20)
    LayerShell.auto_exclusive_zone_enable(window)

    button = Gtk.Button(label="GTK4 Layer Shell with Python")
    button.connect('clicked', lambda x: window.close())
    window.set_child(button)
    window.present()

app = Gtk.Application(application_id='com.github.wmww.gtk4-layer-shell.py-example')
app.connect('activate', on_activate)
app.run(None)
