#!/usr/bin/python3
# When the tests are not enabled, this script serves as a failing test that explains the problem

import sys

color_green = '\x1b[32;1m'
color_normal = '\x1b[0m'
reconfig_command = color_green + 'meson build --reconfigure -Dtests=true' + color_normal
print('you must run ' + reconfig_command + ' in order to run the tests (where build is the path to your build directory)')
exit(1)
