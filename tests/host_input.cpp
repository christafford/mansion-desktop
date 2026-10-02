// Exercise the actual private Wayland listeners as well as camera integration.
#include "src/display.cpp"
#include <cassert>
#include <linux/input-event-codes.h>

static void key(uint32_t code, bool down) {
    keyboard_key(nullptr, nullptr, 0, 0, code,
                 down ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED);
}

int main() {
    // Distinct corners in GL's bottom-first order. Check the exact conversion
    // used to populate the host's shm mapping, including alpha and channels.
    const uint8_t rgba[] = {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 24,
    };
    uint32_t pixels[6]{};
    copy_host_pixels(rgba, pixels, 2, 3);
    assert(pixels[0] == 0x14111213 && pixels[1] == 0x18151617);
    assert(pixels[2] == 0x0c090a0b && pixels[3] == 0x100d0e0f);
    assert(pixels[4] == 0x04010203 && pixels[5] == 0x08050607);

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

    // Render a real room and check the host-export pixels, not the screenshot
    // path (which has its own row flip and did not catch this bug).
    auto* server = wl_display_create();
    auto* compositor = create_compositor(server);
    auto* rendered = create_display_headless(compositor, server);
    assert(rendered);
    rendered->room_mode = true;
    rendered->flat_mode = false;
    rendered->camera.x = 0;
    rendered->camera.y = 2.5f;
    rendered->camera.z = 3;
    rendered->camera.pitch = -10.0f * 3.14159265f / 180.0f;
    render(rendered);
    int w = rendered->window_width, h = rendered->window_height;
    std::vector<uint8_t> readback(w * h * 4);
    std::vector<uint32_t> exported(w * h);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, readback.data());
    assert(glGetError() == GL_NO_ERROR);
    copy_host_pixels(readback.data(), exported.data(), w, h);
    auto matches = [](uint32_t pixel, int r, int g, int b) {
        return std::abs(int((pixel >> 16) & 255) - r) <= 20 &&
               std::abs(int((pixel >> 8) & 255) - g) <= 20 &&
               std::abs(int(pixel & 255) - b) <= 20;
    };
    assert(matches(exported[(h - 1) * w + w / 2], 97, 71, 44));
    assert(matches(exported[w + w / 2], 127, 127, 140));
    if (const char* path = std::getenv("MANSION_HOST_CAPTURE")) {
        FILE* capture = fopen(path, "wb");
        assert(capture);
        fprintf(capture, "P6\n%d %d\n255\n", w, h);
        for (uint32_t pixel : exported) {
            fputc((pixel >> 16) & 255, capture);
            fputc((pixel >> 8) & 255, capture);
            fputc(pixel & 255, capture);
        }
        assert(fclose(capture) == 0);
    }
    destroy_display(rendered);
    destroy_compositor(compositor);
    wl_display_destroy(server);
}
