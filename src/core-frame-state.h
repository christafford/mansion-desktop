#pragma once
#include "frame-snapshot.h"
#include <wayland-server-core.h>

// Only compositor-core creates/uses this state. The legacy surface layout stays
// trivially destructible and never owns these snapshots.
struct CoreFrameState {
    int64_t handle = 0;
    uint64_t revision = 0;
    bool pending_attach = false;
    int pending_scale = 1, pending_transform = 0;
    int scale = 1, transform = 0;
    wl_list committed_callbacks;
    mansion::OwnedFrame frame;
};
