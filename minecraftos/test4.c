#include <stdio.h>
#include <stdint.h>
typedef struct { int v[4]; int is_top_bottom; } Face;
typedef struct { int x, y, z; } Vector3;
static const Face faces[6] = {
    {{0, 1, 2, 3}, 0}, {{5, 4, 7, 6}, 0},
    {{4, 5, 1, 0}, 1}, {{3, 2, 6, 7}, 1},
    {{4, 0, 3, 7}, 0}, {{1, 5, 6, 2}, 0}
};
static const Vector3 cube_vertices[8] = {
    {-50, -50, -50}, { 50, -50, -50}, { 50,  50, -50}, {-50,  50, -50},
    {-50, -50,  50}, { 50, -50,  50}, { 50,  50,  50}, {-50,  50,  50}
};
#include <math.h>
int16_t sin_table[256];
static inline int16_t get_sin(uint8_t angle) { return sin_table[angle]; }
static inline int16_t get_cos(uint8_t angle) { return sin_table[(uint8_t)(angle + 64)]; }

int main() {
    for (int i = 0; i < 256; i++) {
        sin_table[i] = (int16_t)(sin(i * 3.14159265 / 128.0) * 256.0);
    }
    Vector3 proj_3d[8];
    uint8_t angle_x = 30, angle_y = 45, angle_z = 0;
    int sinX = get_sin(angle_x), cosX = get_cos(angle_x);
    int sinY = get_sin(angle_y), cosY = get_cos(angle_y);
    int sinZ = get_sin(angle_z), cosZ = get_cos(angle_z);
    for (int i=0; i<8; i++) {
        int x = cube_vertices[i].x; int y = cube_vertices[i].y; int z = cube_vertices[i].z;
        int xy = (y * cosX - z * sinX) / 256; int xz = (y * sinX + z * cosX) / 256; y = xy; z = xz;
        int yx = (x * cosY + z * sinY) / 256; int yz = (-x * sinY + z * cosY) / 256; x = yx; z = yz;
        int zx = (x * cosZ - y * sinZ) / 256; int zy = (x * sinZ + y * cosZ) / 256; x = zx; y = zy;
        proj_3d[i].x = x; proj_3d[i].y = y; proj_3d[i].z = z;
    }
    for (int i=0; i<6; i++) {
        Vector3 v0 = proj_3d[faces[i].v[0]]; Vector3 v1 = proj_3d[faces[i].v[1]]; Vector3 v2 = proj_3d[faces[i].v[2]];
        int dx1 = v1.x - v0.x; int dy1 = v1.y - v0.y;
        int dx2 = v2.x - v0.x; int dy2 = v2.y - v0.y;
        int normalZ = (dx1 * dy2) - (dy1 * dx2);
        if (normalZ >= 0) {
            printf("Face %d visible, is_top_bottom: %d\n", i, faces[i].is_top_bottom);
        }
    }
    return 0;
}
