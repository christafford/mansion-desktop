// Exercise the actual private Wayland listeners as well as camera integration.
#include "src/display.cpp"
#include <cassert>
#include <linux/input-event-codes.h>

static void key(uint32_t code, bool down) {
    keyboard_key(nullptr, nullptr, 0, 0, code,
                 down ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED);
}

int main() {
    MansionDisplay display{};
    host_window_state host{};
    g_hws = &host;
    keyboard_enter(nullptr, nullptr, 0, nullptr, nullptr);
    key(KEY_W, true);
    input_wayland_apply_movement(&display, 100);
    assert(display.camera.z < 10);
    float z = display.camera.z;
    key(KEY_D, true);
    key(KEY_Q, true); // unrelated events must not clear held navigation keys
    input_wayland_apply_movement(&display, 100);
    assert(display.camera.z < z && display.camera.x > 0);
    z = display.camera.z;
    float x = display.camera.x;
    key(KEY_W, false);
    input_wayland_apply_movement(&display, 100);
    assert(display.camera.z == z && display.camera.x > x);
    key(KEY_D, false);

    // Ordinary pointer motion works when relative-pointer is unavailable.
    pointer_enter(nullptr, nullptr, 0, nullptr, wl_fixed_from_int(100), wl_fixed_from_int(100));
    pointer_motion(nullptr, nullptr, 0, wl_fixed_from_int(110), wl_fixed_from_int(105));
    input_wayland_apply_movement(&display, 16);
    assert(display.camera.yaw < 0 && display.camera.pitch < 0);
    float yaw = display.camera.yaw;
    input_wayland_apply_movement(&display, 16);
    assert(display.camera.yaw == yaw); // consume each delta once
    relative_pointer_motion(nullptr, nullptr, 0, 0, 0, 0,
                            wl_fixed_from_double(0.25), wl_fixed_from_double(0.25));
    input_wayland_apply_movement(&display, 16);
    assert(display.camera.yaw < yaw); // retain subpixel relative motion

    key(KEY_W, true);
    input_wayland_pointer_motion(20, 20);
    keyboard_leave(nullptr, nullptr, 0, nullptr);
    auto camera = display.camera;
    key(KEY_W, true);
    input_wayland_apply_movement(&display, 100);
    assert(display.camera.z == camera.z && display.camera.yaw == camera.yaw);
    keyboard_enter(nullptr, nullptr, 0, nullptr, nullptr);
    input_wayland_apply_movement(&display, 100);
    assert(display.camera.z == camera.z && display.camera.yaw == camera.yaw);

    input_mode_set(InputMode::Application);
    key(KEY_W, true);
    input_wayland_pointer_motion(20, 20);
    input_wayland_apply_movement(&display, 100);
    assert(display.camera.z == camera.z && display.camera.yaw == camera.yaw);
    input_mode_set(InputMode::World);
    input_wayland_apply_movement(&display, 100);
    assert(display.camera.z == camera.z && display.camera.yaw == camera.yaw);
    g_hws = nullptr;
}
