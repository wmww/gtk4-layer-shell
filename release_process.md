# GTK4 Layer Shell release process
1. Make sure you've pulled latest changes from main
1. Make sure the tests pass: `ninja -C build test`
1. Play with gtk4-layer-demo: `build/examples/gtk4-layer-demo` (catch anything the automatic tests missed)
1. Bump version in [meson.build](meson.build)
1. Update [CHANGELOG.md](CHANGELOG.md) ([GitHub compare](https://github.com/wmww/gtk4-layer-shell/compare/) is useful here)
1. Commit and push meson and changelog changes
1. Tag release: `git tag vA.B.C`
1. Push tag: `git push origin vA.B.C`
1. Under Releases in the GitHub repo, the tag should have already appeared, click it
1. Click Edit tag
1. Enter release name (version number, no v prefix) and copy in the changelog
1. Publish release
