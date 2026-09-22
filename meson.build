project('mansion-desktop', 'cpp',
    version: '0.1',
    default_options: ['warning_level=2', 'debug=true', 'cpp_std=c++20'])

# Dependencies
wayland_server = dependency('wayland-server')
wayland_protocols = dependency('wayland-protocols')
egl = dependency('wayland-egl')
dri = dependency('dri')
xkbcommon = dependency('xkbcommon')

# Compiler flags
cpp_args = ['-Wall', '-Wextra', '-Werror']

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
    dependencies: [wayland_server, egl, dri, xkbcommon],
    native: true)
