#!/usr/bin/env python3

import smoke_test_common

bin_path = smoke_test_common.build_dir() + '/examples/session-lock-c'
smoke_test_common.run([bin_path], 'lock', {})
