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
int main() {
    Vector3 proj_3d[8];
    float cx = cos(30 * 3.14159 / 128); float sx = sin(30 * 3.14159 / 128);
    float cy = cos(45 * 3.14159 / 128); float sy = sin(45 * 3.14159 / 128);
    for (int i=0; i<8; i++) {
        float x = cube_vertices[i].x; float y = cube_vertices[i].y; float z = cube_vertices[i].z;
        float xy = y * cx - z * sx; float xz = y * sx + z * cx; y = xy; z = xz;
        float yx = x * cy + z * sy; float yz = -x * sy + z * cy; x = yx; z = yz;
        proj_3d[i].x = x; proj_3d[i].y = y; proj_3d[i].z = z;
    }
    for (int i=0; i<6; i++) {
        Vector3 v0 = proj_3d[faces[i].v[0]];
        Vector3 v1 = proj_3d[faces[i].v[1]];
        Vector3 v2 = proj_3d[faces[i].v[2]];
        float dx1 = v1.x - v0.x; float dy1 = v1.y - v0.y;
        float dx2 = v2.x - v0.x; float dy2 = v2.y - v0.y;
        float normalZ = (dx1 * dy2) - (dy1 * dx2);
        if (normalZ >= 0) {
            printf("Face %d visible, is_top_bottom: %d\n", i, faces[i].is_top_bottom);
        }
    }
    return 0;
}
