#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace mansion {
// Runtime-only identity; never persisted. Pixels are tightly packed, top-left
// buffer coordinates, RGBA8 with straight alpha. Presentation applies the inverse
// buffer transform and scale. No client memory or protocol pointers escape here.
struct FrameSnapshot {
    int64_t handle = 0;
    uint64_t revision = 0;
    bool mapped = false;
    int width = 0, height = 0, stride = 0;
    int logical_width = 0, logical_height = 0;
    int scale = 1, transform = 0;
    // Composite canvas origin relative to the root wl_surface, in logical units.
    int origin_x = 0, origin_y = 0;
    uint32_t source_format = 0;
    int source_stride = 0;
    std::vector<uint8_t> pixels;
};
using OwnedFrame = std::shared_ptr<const FrameSnapshot>;
} // namespace mansion
