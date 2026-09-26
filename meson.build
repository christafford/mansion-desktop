project('mansion-desktop', ['cpp', 'c'],
    version: '0.1',
    default_options: ['warning_level=2', 'debug=true', 'cpp_std=c++20', 'c_std=c11'])

# Dependencies (see docs/decisions/01-stack.md and 02-nested-io-and-verification.md)
wayland_server = dependency('wayland-server')
wayland_client = dependency('wayland-client')
wayland_protocols = dependency('wayland-protocols')
wayland_egl = dependency('wayland-egl')
egl = dependency('egl')
glesv2 = dependency('glesv2')
xkbcommon = dependency('xkbcommon')
x11 = dependency('x11')

# Protocol code generation (xdg-shell is added in docs/TASKS.md P1-T03):
#   wayland_scanner = find_program('wayland-scanner')
#   protocols_dir = wayland_protocols.get_variable('pkgdatadir')
#   xdg_shell_xml = protocols_dir / 'stable/xdg-shell/xdg-shell.xml'
#   custom_target(... wayland_scanner, 'server-header' / 'client-header' / 'private-code' ...)

cpp_args = ['-Wall', '-Wextra']

sources = files(
    'src/main.cpp',
    'src/compositor.cpp',
    'src/shell.cpp',
    'src/display.cpp',
    'src/input.cpp',
    'src/launch.cpp',
)

mansion_exe = executable('mansion-desktop',
    sources,
    cpp_args: cpp_args,
    dependencies: [wayland_server, wayland_egl, egl, glesv2, xkbcommon, x11],
    install: true,
)

# Test client and headless tests (tests/README.md)
test_client = executable('mansion-test-client',
    files('tests/mansion-test-client.c'),
    c_args: ['-Wall', '-Wextra'],
    dependencies: [wayland_client],
)

test_env = environment()
test_env.set('MESON_SOURCE_ROOT', meson.current_source_dir())

test('smoke-headless', find_program('tests/smoke_headless.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('client-globals', find_program('tests/client_globals.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
