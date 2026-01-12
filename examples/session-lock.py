# To run this script without installing the library, set GI_TYPELIB_PATH and LD_LIBRARY_PATH to the build/src directory
# GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src python3 examples/session-lock.py

# For GTK4 Layer Shell to get linked before libwayland-client we must explicitly load it before importing with gi
from ctypes import CDLL
CDLL('libgtk4-layer-shell.so')

import gi
gi.require_version('Gtk', '4.0')
gi.require_version('Gtk4SessionLock', '1.0')

from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import Gtk4SessionLock as SessionLock

class ScreenLock:
    def __init__(self):
        self.lock_instance = SessionLock.Instance.new()
        self.lock_instance.connect('locked', self._on_locked)
        self.lock_instance.connect('unlocked', self._on_unlocked)
        self.lock_instance.connect('failed', self._on_failed)
        self.lock_instance.connect('monitor', self._on_monitor)

    def _on_locked(self, lock_instance):
        print('Locked!')

    def _on_unlocked(self, lock_instance):
        print('Unlocked!')
        app.quit()

    def _on_failed(self, lock_instance):
        print('Failed to lock :(')
        app.quit()

    def _on_monitor(self, lock_instance, monitor):
        window = Gtk.Window(application=app)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        box.set_halign(Gtk.Align.CENTER)
        box.set_valign(Gtk.Align.CENTER)
        window.set_child(box)

        label = Gtk.Label(label="GTK Session Lock with Python")
        box.append(label)

        button = Gtk.Button(label='Unlock')
        button.connect('clicked', self._on_unlock_clicked)
        box.append(button)

        self.lock_instance.assign_window_to_monitor(window, monitor)

    def _on_unlock_clicked(self, button):
        self.lock_instance.unlock()

    def lock(self):
        self.lock_instance.lock()

app = Gtk.Application(application_id='com.github.wmww.gtk4-layer-shell.py-session-lock')
lock = ScreenLock()

def on_activate(app):
    lock.lock()

app.connect('activate', on_activate)
app.run(None)
