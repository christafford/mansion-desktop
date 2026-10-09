#pragma once
#include "frame-snapshot.h"
#include <cstddef>
struct MansionSurface;
struct wl_global;
struct wl_display;
struct wl_resource;

wl_global* surface_tree_create_global(wl_display* display);
void surface_tree_commit(MansionSurface* surface);
void surface_tree_destroyed(MansionSurface* surface);
bool surface_tree_mapped(MansionSurface* surface);
mansion::OwnedFrame surface_tree_snapshot(MansionSurface* surface);
MansionSurface* surface_tree_root(MansionSurface* surface);
// Coordinates are relative to root wl_surface, independent of the canvas origin.
MansionSurface* surface_tree_at(MansionSurface* root, double& x, double& y);
bool surface_tree_coordinates(MansionSurface* root, MansionSurface* target, double& x, double& y);

struct MansionCompositor;
size_t surface_tree_storage(MansionCompositor* compositor);

struct SurfaceTreeBounds { int x, y, width, height; };
SurfaceTreeBounds surface_tree_bounds(MansionSurface* surface);
