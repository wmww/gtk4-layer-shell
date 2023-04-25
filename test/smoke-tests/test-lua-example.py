#!/usr/bin/env python3

import os
import smoke_test_common

script_path = os.path.join(os.path.dirname(__file__), '..', '..', 'examples', 'simple-example.lua')
assert os.path.isfile(script_path), 'script not found at ' + script_path
src_build_dir = smoke_test_common.build_dir() + '/src'
env = {
    'GI_TYPELIB_PATH': src_build_dir,
    'LD_LIBRARY_PATH': src_build_dir,
}
smoke_test_common.run(['luajit', script_path], env)
