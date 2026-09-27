#pragma once

// P4-T01: Room geometry for the one-room end-to-end prototype.
//
// Defines a 10×5×10 room (x:-5..5, y:0..5, z:-5..5) with flat-shaded faces:
//   - brown floor at y=0
//   - four grey walls (back, front, left, right)
//   - light ceiling at y=5
//   - desk at x=-3, z=-3 (dark brown)
//   - monitor frame at x=0, z=-5 (dark grey)
//
// AABB collision keeps the camera inside the room boundaries.

#include <cstddef>

#include <GLES2/gl2.h>

// Room bounding box for AABB collision (all coordinates in world units).
struct RoomBounds {
    float x_min = -4.5f;    // left wall clearance
    float x_max =  4.5f;    // right wall clearance
    float y_min =  0.5f;    // floor clearance (camera must stay above floor)
    float y_max =  6.5f;    // ceiling clearance (room is 7 units tall)
    float z_min = -4.5f;    // front wall clearance
    float z_max =  4.5f;    // back wall clearance
};

// Single mesh (one or more flat-shaded quads) with its draw color.
struct RoomMesh {
    const float* vertices;   // packed: x,y,z,u,v per vertex (5 floats each)
    size_t vertex_count;     // number of vertices
    float color[4];          // RGBA draw color
};

// All room geometry.
struct RoomGeometry {
    RoomBounds bounds;
    RoomMesh meshes[8];      // floor, back wall, front wall, left wall,
                             // right wall, ceiling, desk, monitor frame
    size_t mesh_count;
};

// Initialise the room geometry (sets vertices and draw colors).
void room_geometry_init(RoomGeometry* geo);

// Return a pointer to the immutable geometry.
const RoomGeometry* room_geometry_get(void);

// Apply AABB collision bounds to the camera position.
// Clamp (x, y, z) so it stays within room bounds.
void room_apply_collision(float* x, float* y, float* z);

// Draw the room using the 3D shader (program_3d).
// Must be called after glUseProgram(program_3d) has been set up.
// Pass mvp matrix and the renderer's uniform locations.
void room_draw(const RoomGeometry* geo,
               const float* mvp,
               GLint pos_attr,
               GLint color_uniform);
