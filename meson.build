project('mansion-desktop', 'cpp',
    version: '0.1',
    default_options: ['warning_level=2', 'debug=true', 'cpp_std=c++17'])

# Dependencies
wayland_server = dependency('wayland-server')
wayland_protocols = dependency('wayland-protocols')
wayland_egl = dependency('wayland-egl')
egl = dependency('egl')
xkbcommon = dependency('xkbcommon')
x11 = dependency('x11')
gl = dependency('gl')

# Compiler flags
cpp_args = ['-Wall', '-Wextra']

# Source files
sources = files(
    'src/main.cpp',
    'src/compositor.cpp',
    'src/shell.cpp',
    'src/display.cpp',
    'src/input.cpp',
    'src/launch.cpp',
)

# Executable
executable('mansion-desktop',
    sources,
    cpp_args: cpp_args,
    dependencies: [wayland_server, wayland_egl, egl, xkbcommon, x11, gl],
    native: true,
    link_args: ['-lX11', '-lwayland-client'],
)
