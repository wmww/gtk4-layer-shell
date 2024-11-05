using Gtk;
using GtkLayerShell;

int main(string[] argv) {
    var app = new Gtk.Application (
        "com.github.wmww.gtk4-layer-shell.vala-example",
        GLib.ApplicationFlags.DEFAULT_FLAGS);

    app.activate.connect (() => {
        var window = new Gtk.ApplicationWindow (app);
        GtkLayerShell.init_for_window(window);
        GtkLayerShell.auto_exclusive_zone_enable(window);
        GtkLayerShell.set_margin(window, GtkLayerShell.Edge.TOP, 10);
        GtkLayerShell.set_margin(window, GtkLayerShell.Edge.BOTTOM, 10);
        GtkLayerShell.set_anchor(window, GtkLayerShell.Edge.BOTTOM, true);
        var button = new Gtk.Button.with_label ("Hello, World!");
        button.clicked.connect (() => {
            window.close ();
        });
        window.set_child (button);
        window.present ();
    });

    return app.run (argv);
}
