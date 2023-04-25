-- To run this script without installing the library, set GI_TYPELIB_PATH and LD_LIBRARY_PATH to the build/src directory
-- GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src luajit examples/simple-example.lua

-- For GTK4 Layer Shell to get linked before libwayland-client we must explicitly load it before importing with lgi
local ffi = require("ffi")
ffi.cdef[[
    void *dlopen(const char *filename, int flags);
]]
ffi.C.dlopen("libgtk4-layer-shell.so", 0x101)

-- Now open the library normally with LGI
local lgi = require("lgi")
local Gtk = lgi.require("Gtk", "4.0")
local layer_shell = lgi.require("Gtk4LayerShell")
local app = Gtk.Application {
   application_id = "com.github.a-cloud-ninja.gtk4-layer-shell.lua-example",
}
app.on_activate = function()
   local win = Gtk.Window {
      application = app
   }
   layer_shell.init_for_window(win)
   layer_shell.set_layer(win, 2)
   layer_shell.set_anchor(win, 1)
   layer_shell.set_exclusive_zone(win, 0)
   local button = Gtk.Button {
      label = "Gtk4 Layer Shell Example",
   }
   button.on_clicked = function()
      win:close()
   end
   win:set_child(button)
   win:present()
end
app:run()
