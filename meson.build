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

cpp_args = ['-Wall', '-Wextra']

# Protocol code is generated at build time, never committed (P1-T03).
wayland_scanner = find_program('wayland-scanner')
wp_dir = wayland_protocols.get_variable(pkgconfig: 'pkgdatadir')
xdg_shell_xml = wp_dir / 'stable' / 'xdg-shell' / 'xdg-shell.xml'
xdg_shell_server_h = custom_target('xdg-shell-server-header',
    input: xdg_shell_xml, output: 'xdg-shell-server-protocol.h',
    command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'])
xdg_shell_client_h = custom_target('xdg-shell-client-header',
    input: xdg_shell_xml, output: 'xdg-shell-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'])
xdg_shell_code = custom_target('xdg-shell-code',
    input: xdg_shell_xml, output: 'xdg-shell-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'])

sources = files(
    'src/main.cpp',
    'src/compositor.cpp',
    'src/xdg-shell.cpp',
    'src/display.cpp',
    'src/input.cpp',
    'src/launch.cpp',
    'src/room.cpp',
)

mansion_exe = executable('mansion-desktop',
    sources, xdg_shell_server_h, xdg_shell_code,
    cpp_args: cpp_args,
    dependencies: [wayland_server, wayland_egl, egl, glesv2, xkbcommon, x11],
    include_directories: include_directories('.'),
    install: true,
)

# Test client and headless tests (tests/README.md)
test_client = executable('mansion-test-client',
    files('tests/mansion-test-client.c'), xdg_shell_client_h, xdg_shell_code,
    c_args: ['-Wall', '-Wextra'],
    dependencies: [wayland_client, egl, glesv2],
)

test_env = environment()
test_env.set('MESON_SOURCE_ROOT', meson.current_source_dir())

# Disable leak detection for ASan tests. The wayland-server library does not
# free its internal data structures on wl_display_destroy(), so short-lived
# compositor processes always leak a small amount of wayland-internal memory.
# All other ASan checks (use-after-free, buffer overflow, etc.) remain active.
test_env.set('ASAN_OPTIONS', 'detect_leaks=0:halt_on_error=1:abort_on_error=1:print_summary=1:print_stacktrace=1')
test_env.set('MSAN_OPTIONS', 'detect_leaks=0:halt_on_error=1:abort_on_error=1:print_summary=1:print_stacktrace=1')

test('smoke-headless', find_program('tests/smoke_headless.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('client-globals', find_program('tests/client_globals.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('client-toplevel', find_program('tests/client_toplevel.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('client-frame', find_program('tests/client_frame.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('render-shm', find_program('tests/render_shm.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('input-routing', find_program('tests/input_routing.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('launch-client', find_program('tests/launch_client.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)
test('lifecycle', find_program('tests/lifecycle.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p1', timeout: 30)

test('render-panel', find_program('tests/render_panel.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p2', timeout: 30)
test('render-update', find_program('tests/render-update.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p2', timeout: 45)
test('render-lifecycle', find_program('tests/render_lifecycle.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p2', timeout: 45)

test('render-egl', find_program('tests/render_egl.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p2', timeout: 45)

test('camera-move', find_program('tests/camera-move.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p2', timeout: 60)

test('mode-routing', find_program('tests/mode_routing.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p3', timeout: 30)

test('mode-exit', find_program('tests/mode_exit.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p3', timeout: 30)

test('focus-loss', find_program('tests/focus_loss.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p3', timeout: 30)

test('present-fullsize', find_program('tests/present_fullsize.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p3', timeout: 30)

test('select-panel', find_program('tests/select_panel.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p3', timeout: 30)

test('app-exit', find_program('tests/app_exit.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p3', timeout: 45)

test('room-render', find_program('tests/room_render.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p4', timeout: 30)
test('room-collision', find_program('tests/room_collision.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p4', timeout: 45)

# P2-T01: header-only math module unit test
math_test = executable('test_math',
    'tests/test_math.cpp',
    cpp_args: cpp_args,
    install: false,
)

test('math', math_test, suite: 'p2', timeout: 10)
