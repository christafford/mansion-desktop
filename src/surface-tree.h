#pragma once
#include "frame-snapshot.h"
#include <cstddef>
struct ElsewhereSurface;
struct wl_global;
struct wl_display;
struct wl_resource;

wl_global* surface_tree_create_global(wl_display* display);
void surface_tree_commit(ElsewhereSurface* surface);
void surface_tree_destroyed(ElsewhereSurface* surface);
bool surface_tree_mapped(ElsewhereSurface* surface);
elsewhere::OwnedFrame surface_tree_snapshot(ElsewhereSurface* surface);
ElsewhereSurface* surface_tree_root(ElsewhereSurface* surface);
// Coordinates are relative to root wl_surface, independent of the canvas origin.
ElsewhereSurface* surface_tree_at(ElsewhereSurface* root, double& x, double& y);
bool surface_tree_coordinates(ElsewhereSurface* root, ElsewhereSurface* target, double& x, double& y);

struct ElsewhereCompositor;
size_t surface_tree_storage(ElsewhereCompositor* compositor);

struct SurfaceTreeBounds { int x, y, width, height; };
SurfaceTreeBounds surface_tree_bounds(ElsewhereSurface* surface);
