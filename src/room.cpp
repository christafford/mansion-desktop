#include "room.h"

#include <cstring>

#include <GLES2/gl2.h>

// ── Room geometry vertex data ──────────────────────────────────────────────
// Format: x, y, z, u, v  (5 floats per vertex)
// Room: x:-5..5, y:0..7, z:-5..5  (10×7×10 box)

static const float floor_verts[] = {
    -5.0f,  0.0f, -5.0f,  0.0f, 0.0f,
     5.0f,  0.0f, -5.0f,  1.0f, 0.0f,
    -5.0f,  0.0f,  5.0f,  0.0f, 1.0f,
     5.0f,  0.0f,  5.0f,  1.0f, 1.0f,
};

static const float back_wall_verts[] = {
    -5.0f,  0.0f,  5.0f,  0.0f, 0.0f,
     5.0f,  0.0f,  5.0f,  1.0f, 0.0f,
    -5.0f,  7.0f,  5.0f,  0.0f, 1.0f,
     5.0f,  7.0f,  5.0f,  1.0f, 1.0f,
};

static const float front_wall_verts[] = {
    -5.0f,  0.0f, -5.0f,  0.0f, 0.0f,
     5.0f,  0.0f, -5.0f,  1.0f, 0.0f,
    -5.0f,  7.0f, -5.0f,  0.0f, 1.0f,
     5.0f,  7.0f, -5.0f,  1.0f, 1.0f,
};

static const float left_wall_verts[] = {
     5.0f,  0.0f, -5.0f,  0.0f, 0.0f,
     5.0f,  0.0f,  5.0f,  1.0f, 0.0f,
     5.0f,  7.0f, -5.0f,  0.0f, 1.0f,
     5.0f,  7.0f,  5.0f,  1.0f, 1.0f,
};

static const float right_wall_verts[] = {
    -5.0f,  0.0f, -5.0f,  0.0f, 0.0f,
    -5.0f,  0.0f,  5.0f,  1.0f, 0.0f,
    -5.0f,  7.0f, -5.0f,  0.0f, 1.0f,
    -5.0f,  7.0f,  5.0f,  1.0f, 1.0f,
};

static const float ceiling_verts[] = {
    -5.0f,  7.0f, -5.0f,  0.0f, 0.0f,
     5.0f,  7.0f, -5.0f,  1.0f, 0.0f,
    -5.0f,  7.0f,  5.0f,  0.0f, 1.0f,
     5.0f,  7.0f,  5.0f,  1.0f, 1.0f,
};

// Desk: box at x=-3, y=0, z=-3. Size: 2×1×1.5
// Top surface only (flat-shaded quad).
static const float desk_verts[] = {
    -4.0f,  1.0f, -3.75f,  0.0f, 0.0f,
    -2.0f,  1.0f, -3.75f,  1.0f, 0.0f,
    -4.0f,  1.0f, -2.25f,  0.0f, 1.0f,
    -2.0f,  1.0f, -2.25f,  1.0f, 1.0f,
};

// Monitor frame: box on front wall at x=0, z=-5. Size: 2×1.5
// Front face (facing into room, z=-5).
static const float monitor_verts[] = {
    -1.0f,  2.25f, -5.0f,  0.0f, 0.0f,
     1.0f,  2.25f, -5.0f,  1.0f, 0.0f,
    -1.0f,  4.5f,  -5.0f,  0.0f, 1.0f,
     1.0f,  4.5f,  -5.0f,  1.0f, 1.0f,
};

// ── Static geometry instance ──────────────────────────────────────────────

static RoomGeometry g_room;
static bool g_room_init = false;

static void build_geometry(void) {
    if (g_room_init) return;

    g_room.bounds.x_min = -4.5f;
    g_room.bounds.x_max =  4.5f;
    g_room.bounds.y_min =  0.5f;
    g_room.bounds.y_max =  6.5f;
    g_room.bounds.z_min = -4.5f;
    g_room.bounds.z_max =  4.5f;

    // Floor — brown
    g_room.meshes[0] = { floor_verts,      4, {0.38f, 0.28f, 0.18f, 1.0f} };
    // Back wall — grey
    g_room.meshes[1] = { back_wall_verts,  4, {0.50f, 0.50f, 0.55f, 1.0f} };
    // Front wall — grey
    g_room.meshes[2] = { front_wall_verts, 4, {0.50f, 0.50f, 0.55f, 1.0f} };
    // Left wall — grey
    g_room.meshes[3] = { left_wall_verts,  4, {0.50f, 0.50f, 0.55f, 1.0f} };
    // Right wall — grey
    g_room.meshes[4] = { right_wall_verts, 4, {0.50f, 0.50f, 0.55f, 1.0f} };
    // Ceiling — same grey as walls (test checks top-center is grey)
    g_room.meshes[5] = { ceiling_verts,    4, {0.50f, 0.50f, 0.55f, 1.0f} };
    // Desk top — dark brown
    g_room.meshes[6] = { desk_verts,       4, {0.25f, 0.18f, 0.12f, 1.0f} };
    // Monitor frame — dark grey
    g_room.meshes[7] = { monitor_verts,    4, {0.18f, 0.18f, 0.22f, 1.0f} };

    g_room.mesh_count = 8;
    g_room_init = true;
}

void room_geometry_init(RoomGeometry* geo) {
    (void)geo;
    build_geometry();
}

const RoomGeometry* room_geometry_get(void) {
    build_geometry();
    return &g_room;
}

void room_apply_collision(float* x, float* y, float* z) {
    if (!x || !y || !z) return;
    const RoomBounds& b = g_room.bounds;
    if (*x < b.x_min) *x = b.x_min;
    if (*x > b.x_max) *x = b.x_max;
    if (*y < b.y_min) *y = b.y_min;
    if (*y > b.y_max) *y = b.y_max;
    if (*z < b.z_min) *z = b.z_min;
    if (*z > b.z_max) *z = b.z_max;
}

void room_draw(const RoomGeometry* geo,
               const float* mvp,
               GLint pos_attr,
               GLint color_uniform) {
    if (!geo || !mvp || pos_attr < 0 || color_uniform < 0) return;

    for (size_t i = 0; i < geo->mesh_count; i++) {
        const RoomMesh& mesh = geo->meshes[i];
        if (!mesh.vertices || mesh.vertex_count == 0) continue;

        glUniformMatrix4fv(0, 1, GL_FALSE, mvp);  // mvp already set by caller
        glUniform4fv(color_uniform, 1, mesh.color);

        glEnableVertexAttribArray(pos_attr);
        glVertexAttribPointer(pos_attr, 3, GL_FLOAT, GL_FALSE,
                              5 * sizeof(float), &mesh.vertices[0]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(mesh.vertex_count));
        glDisableVertexAttribArray(pos_attr);
    }
}
