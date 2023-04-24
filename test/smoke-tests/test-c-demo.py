#!/usr/bin/env python3

import smoke_test_common

bin_path = smoke_test_common.build_dir() + '/examples/gtk4-layer-demo'
args = [bin_path] + '-l top -a lrb -m 20,20,20,0 -e -k on-demand'.split()
smoke_test_common.run(args, {})
