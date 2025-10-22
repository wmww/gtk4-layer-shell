import subprocess
import os
import sys
from typing import List, Dict

timeout = 1

def build_dir() -> str:
    p = os.environ.get('GTKLS_BUILD_DIR')
    assert p, 'GTKLS_BUILD_DIR environment variable not set'
    assert os.path.isdir(p), p + ' (from GTKLS_BUILD_DIR) does not exist'
    return p

def expect(*args) -> None:
    print('EXPECT: ' + ' '.join(args), file=sys.stderr)

def run(cmd: List[str], mode: str, env: Dict[str, str]) -> None:
    if mode == 'layer':
        expect('zwlr_layer_shell_v1', '.get_layer_surface')
        expect('wl_surface', '.commit')
    elif mode == 'lock':
        expect('ext_session_lock_manager_v1', '.lock');
        expect('ext_session_lock_v1', '.get_lock_surface');
        expect('ext_session_lock_v1', '.locked');
    else:
        assert False, 'Invalid mode ' + str(mode)
    try:
        result = subprocess.run(cmd, env={**os.environ, **env}, timeout=timeout)
        assert False, 'subprocess completed without timeout expiring, return code: ' + str(result.returncode)
    except subprocess.TimeoutExpired:
        pass
