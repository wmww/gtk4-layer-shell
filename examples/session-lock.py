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

def on_locked(lock_instance):
    print('Locked!')

def on_unlocked(lock_instance):
    print('Unlocked!')
    app.quit()

def on_failed(lock_instance):
    print('Failed to lock :(')
    app.quit()

def create_lock_window(lock_instance, monitor):
    window = Gtk.Window(application=app)

    box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
    box.set_halign(Gtk.Align.CENTER)
    box.set_valign(Gtk.Align.CENTER)
    window.set_child(box)

    label = Gtk.Label(label="GTK Session Lock with Python")
    box.append(label)

    def on_unlock_clicked(button):
        lock_instance.unlock()

    button = Gtk.Button(label='Unlock')
    button.connect('clicked', on_unlock_clicked)
    box.append(button)

    lock_instance.assign_window_to_monitor(window, monitor)
    window.present()

def on_activate(app):
    lock_instance = SessionLock.Instance.new()
    lock_instance.connect('locked', on_locked)
    lock_instance.connect('unlocked', on_unlocked)
    lock_instance.connect('failed', on_failed)

    if not lock_instance.lock():
        # Failure has already been handled in on_failed()
        return

    display = Gdk.Display.get_default()

    for monitor in display.get_monitors():
        create_lock_window(lock_instance, monitor)

app = Gtk.Application(application_id='com.github.wmww.gtk4-layer-shell.py-session-lock')
app.connect('activate', on_activate)
app.run(None)
