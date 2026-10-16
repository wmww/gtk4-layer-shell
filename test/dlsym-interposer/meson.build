dlsym_interposer_lib = shared_library(
    'dlsym-interposer',
    files('dlsym-interposer.c'))

dlsym_interposer = declare_dependency(
    include_directories: include_directories('.'),
    link_with: dlsym_interposer_lib)
