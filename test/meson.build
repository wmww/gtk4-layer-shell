subdir('test-common')
subdir('mock-server')
subdir('integration-test-common')
subdir('layer-tests')
subdir('lock-tests')
subdir('unit-tests')

py = find_program('python3')
run_test_script = files(meson.current_source_dir() + '/run-integration-test.py')

env = environment()
env.set('GTK4_LAYER_SHELL_BUILD', meson.build_root())

foreach test_list : [['layer', layer_tests], ['lock', lock_tests]]
    test_prefix = test_list[0]
    test_dir = test_prefix + '-tests'
    foreach test_name : test_list[1]
        test_full_name = test_prefix + '-' + test_name
        exe = executable(
            test_full_name,
            files(join_paths(test_dir, test_name + '.c')),
            dependencies: [gtk, wayland_client, gtk_layer_shell, integration_test_common])
        test(
            test_full_name,
            py,
            workdir: meson.current_source_dir(),
            env: env,
            args: [
                run_test_script,
                meson.current_build_dir() + '/' + test_full_name,
            ])
    endforeach
endforeach

if get_option('smoke-tests')
    subdir('smoke-tests')
    foreach smoke_test : smoke_tests
        test(
            'smoke-' + smoke_test,
            py,
            workdir: meson.current_source_dir(),
            env: env,
            args: [
                run_test_script,
                meson.current_source_dir() + '/smoke-tests/' + smoke_test + '.py',
            ])
    endforeach
endif

check_tests_in_meson_script = files(meson.current_source_dir() + '/check-all-tests-are-in-meson.py')
test('check-all-tests-are-in-meson', py, args: [check_tests_in_meson_script])
