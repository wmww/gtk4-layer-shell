#!/usr/bin/python3
# Tests need to be added to meson.build. This script makes sure they are.

import os
from os import path
import logging

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.WARNING)

dead_tests = []

def check_dir(dir_path):
    logger.info('checking ' + dir_path)
    assert path.isdir(dir_path)
    meson_path = path.join(dir_path, 'meson.build')
    assert path.isfile(meson_path)
    with open(meson_path, 'r') as f:
        meson = f.read()
    for filename in os.listdir(dir_path):
        root, ext = path.splitext(filename)
        if root.startswith('test-'):
            search_str = "'" + root + "'"
            if search_str in meson:
                logger.info(search_str + ' is in meson')
            else:
                logger.info(search_str + ' is not in meson')
                dead_tests.append(path.join(dir_path, filename))
        else:
            logger.info(filename + ' ignored')

if __name__ == '__main__':
    test_dir = path.dirname(path.realpath(__file__))
    check_dir(path.join(test_dir, 'integration-tests'))
    check_dir(path.join(test_dir, 'smoke-tests'))
    check_dir(path.join(test_dir, 'unit-tests'))
    if dead_tests:
        print('The following test(s) have not been added to meson:')
        for test in dead_tests:
            print('  ' + test)
        exit(1)
