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

relative_ptr_xml = wp_dir / 'unstable' / 'relative-pointer' / 'relative-pointer-unstable-v1.xml'
relative_ptr_client_h = custom_target('relative-pointer-client-header',
    input: relative_ptr_xml, output: 'relative-pointer-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'])
relative_ptr_code = custom_target('relative-pointer-code',
    input: relative_ptr_xml, output: 'relative-pointer-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'])

# Source files for the main executable (display layer)
sources_display = files(
    'src/main.cpp',
    'src/compositor.cpp',
    'src/display.cpp',
    'src/input.cpp',
    'src/launch.cpp',
    'src/room.cpp',
)

# Protocol ownership is shared by the legacy frontend and the Godot runtime.
compositor_protocol_lib = static_library('compositor-protocol',
    'src/seat.cpp', 'src/xdg-shell.cpp', xdg_shell_server_h, xdg_shell_code,
    cpp_args: cpp_args, pic: true,
    dependencies: [wayland_server, xkbcommon], install: false)

# Core compositor library (PIC, no GL/display dependencies)
compositor_core_lib = static_library('compositor-core',
    'src/compositor-core.cpp', 'src/surface-tree.cpp',
    link_with: compositor_protocol_lib,
    cpp_args: cpp_args + ['-fPIC'],
    dependencies: [wayland_server],
    include_directories: include_directories('.'),
    install: false,
)

# Public header for core library
compositor_core_hdr = files('src/compositor-core.h')

# GDExtension adapter for Godot integration
# Note: godot-cpp must be built first at tools/godot-cpp/build/
godot_cpp_include = include_directories('tools/godot-cpp/include', 'tools/godot-cpp/build/gen/include')
godot_include = include_directories('tools/godot-4.7.2-stable/core/extension')

# GDExtension source files - use compositor-core API, not old bridge
gdextension_sources = files(
    'src/godot/mansion_extension.cpp',
    'src/godot/mansion_gdextension_adapter.cpp',
)

# GDExtension shared library
# The .gdextension resource file references this .so by name
# Note: GDExtension libraries load symbols at runtime via godot_extension_get_library_symbol
# We link against compositor-core static library for the protocol layer,
# but NOT against libgodot-cpp.a (symbols resolve at runtime)
# Note: wayland-client is needed for wl_display_dispatch_pending used in pump()
# Note: -fvisibility=hidden hides symbols by default, so we need explicit visibility
# for exported functions. GDExtensionInit must be visible.
mansion_gdextension = shared_library('mansion_godot',
    gdextension_sources,
    cpp_args: cpp_args + ['-fPIC', '-std=c++17', '-fvisibility=hidden'],
    include_directories: [include_directories('src'), godot_cpp_include, godot_include],
    dependencies: [wayland_server, wayland_client],
    link_with: [compositor_core_lib],
    install: false,
)

mansion_exe = executable('mansion-desktop',
    sources_display, xdg_shell_server_h, xdg_shell_code, xdg_shell_client_h,
    relative_ptr_client_h, relative_ptr_code,
    cpp_args: cpp_args,
    dependencies: [wayland_server, wayland_client, wayland_egl, egl, glesv2, xkbcommon],
    include_directories: include_directories('.'),
    link_with: [compositor_core_lib, compositor_protocol_lib, mansion_gdextension],
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
test('render-mixed', find_program('tests/render_mixed.sh'),
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
test('p4-surface-state', find_program('tests/p4_surface_state.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p4', timeout: 60)
test('xdg-shell', find_program('tests/xdg_shell.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p4', timeout: 30)
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

test('teleport', find_program('tests/teleport.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p4', timeout: 45)

test('milestone1', find_program('tests/milestone1.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p4', timeout: 60)

test('focus-cycle', find_program('tests/focus_cycle.sh'),
    args: [mansion_exe, test_client], env: test_env, suite: 'p5', timeout: 30)

# P2-T01: header-only math module unit test
math_test = executable('test_math',
    'tests/test_math.cpp',
    cpp_args: cpp_args,
    install: false,
)

test('math', math_test, suite: 'p2', timeout: 10)

host_events_test = executable('test_host_events', 'tests/host_events.cpp',
    cpp_args: cpp_args,
    include_directories: include_directories('.'),
    dependencies: [wayland_client, wayland_server])
test('host-events', host_events_test, suite: 'p4', timeout: 5)

host_input_test = executable('test_host_input',
    'tests/host_input.cpp', 'src/input.cpp', 'src/compositor.cpp',
    'src/room.cpp', 'src/launch.cpp',
    xdg_shell_server_h, xdg_shell_client_h, xdg_shell_code,
    relative_ptr_client_h, relative_ptr_code,
    link_with: compositor_protocol_lib,
    cpp_args: cpp_args,
    include_directories: include_directories('.'),
    dependencies: [wayland_server, wayland_client, wayland_egl, egl, glesv2, xkbcommon])
test('host-input', host_input_test, suite: 'p4', timeout: 5)

# Core library test
compositor_core_test = executable('test_compositor_core',
    'tests/test_compositor_core.c',
    cpp_args: cpp_args,
    include_directories: [include_directories('src')],
    link_with: [compositor_core_lib])
test('compositor-core', compositor_core_test, suite: 'p2', timeout: 10)


# Real socket/resource lifecycle, also exercised through the Godot binding.
runtime_client = executable('runtime-client',
    'tests/runtime-client.c', xdg_shell_client_h, xdg_shell_code,
    dependencies: [wayland_client, xkbcommon])
runtime_lifecycle_test = executable('test_compositor_runtime',
    'tests/test_compositor_runtime.cpp', 'src/compositor-runtime.cpp',
    cpp_args: cpp_args, include_directories: include_directories('src'),
    dependencies: wayland_server, link_with: compositor_core_lib)
test('compositor-runtime', runtime_lifecycle_test,
    args: [runtime_client.full_path()], suite: 'p21', timeout: 30)

frame_client = executable('frame-client',
    'tests/frame-client.c', xdg_shell_client_h, xdg_shell_code,
    c_args: ['-Wall', '-Wextra'], dependencies: wayland_client)
frame_snapshot_test = executable('test_frame_snapshot',
    'tests/test_frame_snapshot.cpp', 'src/compositor-runtime.cpp',
    cpp_args: cpp_args, include_directories: include_directories('src'),
    dependencies: wayland_server, link_with: compositor_core_lib)
test('frame-snapshot', frame_snapshot_test,
    args: [frame_client.full_path()], suite: 'p21', timeout: 45)

surface_tree_test = executable('test_surface_tree',
    'tests/test_surface_tree.cpp', 'src/compositor-runtime.cpp', xdg_shell_client_h,
    link_with: compositor_core_lib, dependencies: [wayland_client, wayland_server, xkbcommon],
    cpp_args: cpp_args, include_directories: include_directories('src'))
test('surface-tree', surface_tree_test, suite: 'p21', timeout: 30)
