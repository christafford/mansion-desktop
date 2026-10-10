#pragma once
#include "frame-snapshot.h"
#include <wayland-server-core.h>
#include <optional>

struct ElsewhereSurface;
struct ElsewhereSubsurface;
struct InputRectangle { int32_t x, y, width, height; bool subtract; };
using InputRegion = std::optional<std::vector<InputRectangle>>;

// Owned by compositor-core and its surface-tree boundary. The legacy layout stays
// trivially destructible and never owns these snapshots.
struct CoreFrameState {
    int64_t handle = 0;
    uint64_t revision = 0;
    bool pending_attach = false;
    int pending_scale = 1, pending_transform = 0;
    int scale = 1, transform = 0;
    wl_list committed_callbacks;
    elsewhere::OwnedFrame frame;
    // A synchronized child's committed state stays cached until its parent
    // applies it. Buffer ownership is still released immediately after copying.
    elsewhere::OwnedFrame visible_frame;
    wl_list cached_callbacks;
    bool cached_commit = false;
    InputRegion pending_input, cached_input, visible_input;
    bool subsurface_role = false;
    ElsewhereSubsurface* subsurface = nullptr;
    std::vector<ElsewhereSurface*> order, pending_order, cached_order;
    bool tree_used = false;
    elsewhere::OwnedFrame composite;
    std::vector<int64_t> composite_signature;
    uint64_t composite_revision = 0;
};
