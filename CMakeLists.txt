cmake_minimum_required(VERSION 3.16)
project(mansion-desktop VERSION 0.1 LANGAGES CXX)

set(CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

# Find packages (will fail until dependencies are installed)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND REQUIRED wayland-server wayland-client wayland-egl)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

add_executable(mansion-desktop
    src/main.cpp
    src/compositor.cpp
    src/display.cpp
    src/input.cpp
    src/launch.cpp
)

target_link_libraries(mansion-desktop
    PRIVATE
        Wayland::wayland_server
        Wayland::wayland_client
        Wayland::wayland-egl
        EGL::EGL
        XKBCOMMON::xkbcommon
)

# Placeholder target that works without dependencies
add_executable(mansion-desktop-placeholder src/main.cpp)
