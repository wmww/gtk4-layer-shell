# Layer Shell Preload
`liblayer-shell-preload.so` is a hack to allow arbitrary Wayland apps to use the Layer Shell protocol. It uses the same approach as gtk4-layer-shell, but generalized to work with any `libwayland-client` program. It's designed to be `LD_PRELOAD`ed into pre-built binaries, no recompiling necessary.

There is no officially supported way to use Layer Shell Preload. If it works, it works. If it doesn't work or randomly stops working, that is a you problem. Not a me problem, and definitely not an upstream app dev problem. __Do not report bugs to app or toolkit developers if Layer Shell Preload is involved in any way.__ You're welcome to report bugs here, but I'm likely to ignore or close them (especially it's just "crashes X program").

## Usage
Run your program with `LD_PRELOAD` set to the path to `liblayer-shell-preload.so`, and use `LAYER_` environment variables to control the layer surface's properties. For example
```
$ LD_PRELOAD=/usr/lib/liblayer-shell-preload.so LAYER_ANCHOR='lrb' LAYER_HEIGHT=50 LAYER_EXCLUSIVE=1 LAYER_KEYBOARD=on-demand weston-terminal
```
The options are
| Variable | Values | Use |
| - | - | - |
| `LAYER_LAYER` | unset/`o`/`overlay`, `t`/`top`, `b`/`bottom`, `g`/`background` | The layer of the window (overlay and top are over other windows, bottom and background are below them) |
| `LAYER_ANCHOR` | One or more of the letters `l`, `r`, `t` and `b` in any order, or `0`/unset for none | Which edges of the screen to anchor the surface to |
| `LAYER_EXCLUSIVE` | unset/`0` or `1` | If to take up space and push other windows out of the way. Only works if anchored to an edge |
| `LAYER_WIDTH`/`LAYER_HEIGHT` | unset or any number | The preferred width and/or height for the surface when it's not being stretched across the screen |
| `LAYER_KEYBOARD` | unset/`n`/`none`, `e`/`exclusive` or `o`/`on-demand` | If to allow or lock keyboard focus on the window |
| `LAYER_NAMESPACE` | unset or any string | The `namespace` property for the layer shell surface, can be used by the Wayland compositor |
| `LAYER_ALL_SURFACES` | unset/`0` or `1` | By default only the first surface a program creates is a layer surface. With this set they all are. This breaks popups |

Output selection is not yet supported, but could be added in the future.
