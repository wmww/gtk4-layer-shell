subdir('test-common')
subdir('mock-server')
subdir('integration-test-common')
subdir('integration-tests')
subdir('unit-tests')

py = find_program('python3')
run_test_script = files(meson.current_source_dir() + '/run-integration-test.py')

env = environment()
env.set('GTK4_LAYER_SHELL_BUILD', meson.build_root())

foreach integration_test : integration_tests
    integration_test_srcs = files('integration-tests/' + integration_test + '.c')
    exe = executable(
        integration_test,
        integration_test_srcs,
        dependencies: [gtk, wayland_client, gtk_layer_shell, integration_test_common])
    test(
        'integration-' + integration_test,
        py,
        workdir: meson.current_source_dir(),
        env: env,
        args: [
            run_test_script,
            meson.current_build_dir() + '/' + integration_test,
        ])
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
