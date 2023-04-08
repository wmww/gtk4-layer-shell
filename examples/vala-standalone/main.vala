using Gtk;
using GtkLayerShell;

int main(string[] args) {
    Gtk.init(ref args);
    var window = new Window();
    var label = new Label("GTK Layer Shell with Vala!");
    window.add(label);
    GtkLayerShell.init_for_window(window);
    GtkLayerShell.auto_exclusive_zone_enable(window);
    GtkLayerShell.set_margin(window, GtkLayerShell.Edge.TOP, 10);
    GtkLayerShell.set_margin(window, GtkLayerShell.Edge.BOTTOM, 10);
    GtkLayerShell.set_anchor(window, GtkLayerShell.Edge.BOTTOM, true);
    window.destroy.connect(Gtk.main_quit);
    window.show_all();
    Gtk.main();
    return 0;
}
