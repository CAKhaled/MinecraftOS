#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define WIDTH 320
#define HEIGHT 200
uint8_t backbuffer[WIDTH * HEIGHT];
typedef struct { int x, y, u, v; } Vertex2D;
typedef struct { int x, y, z; } Vector3;
typedef struct { int v[4]; int is_top_bottom; } Face;
static const Face faces[6] = {
    {{0, 1, 2, 3}, 0}, {{5, 4, 7, 6}, 0},
    {{4, 5, 1, 0}, 1}, {{3, 2, 6, 7}, 1},
    {{4, 0, 3, 7}, 0}, {{1, 5, 6, 2}, 0}
};
static const Vector3 cube_vertices[8] = {
    {-50, -50, -50}, { 50, -50, -50}, { 50,  50, -50}, {-50,  50, -50},
    {-50, -50,  50}, { 50, -50,  50}, { 50,  50,  50}, {-50,  50,  50}
};
static int edge_function(Vertex2D a, Vertex2D b, Vertex2D p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}
static void draw_triangle(Vertex2D v0, Vertex2D v1, Vertex2D v2, uint8_t color) {
    int minX = v0.x; if (v1.x < minX) minX = v1.x; if (v2.x < minX) minX = v2.x;
    int maxX = v0.x; if (v1.x > maxX) maxX = v1.x; if (v2.x > maxX) maxX = v2.x;
    int minY = v0.y; if (v1.y < minY) minY = v1.y; if (v2.y < minY) minY = v2.y;
    int maxY = v0.y; if (v1.y > maxY) maxY = v1.y; if (v2.y > maxY) maxY = v2.y;
    if (minX < 0) minX = 0; if (maxX >= WIDTH) maxX = WIDTH - 1;
    if (minY < 0) minY = 0; if (maxY >= HEIGHT) maxY = HEIGHT - 1;
    int area = edge_function(v0, v1, v2);
    if (area == 0) return;
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Vertex2D p = {x, y, 0, 0};
            int w0 = edge_function(v1, v2, p);
            int w1 = edge_function(v2, v0, p);
            int w2 = edge_function(v0, v1, p);
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                backbuffer[y * WIDTH + x] = color;
            }
        }
    }
}
int main() {
    for(int i=0; i<WIDTH*HEIGHT; i++) backbuffer[i] = 0;
    int angle_x = 45, angle_y = 0, angle_z = 0;
    int16_t sin_table[256];
    for (int i = 0; i < 256; i++) sin_table[i] = (int16_t)(sin(i * 3.14159265 / 128.0) * 256.0);
    int sinX = sin_table[angle_x], cosX = sin_table[(angle_x + 64) % 256];
    int sinY = sin_table[angle_y], cosY = sin_table[(angle_y + 64) % 256];
    int sinZ = sin_table[angle_z], cosZ = sin_table[(angle_z + 64) % 256];
    Vector3 proj_3d[8];
    Vertex2D screen[8];
    for (int i = 0; i < 8; i++) {
        int x = cube_vertices[i].x; int y = cube_vertices[i].y; int z = cube_vertices[i].z;
        int xy = (y * cosX - z * sinX) / 256; int xz = (y * sinX + z * cosX) / 256; y = xy; z = xz;
        int yx = (x * cosY + z * sinY) / 256; int yz = (-x * sinY + z * cosY) / 256; x = yx; z = yz;
        int zx = (x * cosZ - y * sinZ) / 256; int zy = (x * sinZ + y * cosZ) / 256; x = zx; y = zy;
        proj_3d[i].x = x; proj_3d[i].y = y; proj_3d[i].z = z;
        z += 150;
        screen[i].x = (x * 256 / z) + (WIDTH / 2); screen[i].y = (y * 256 / z) + (HEIGHT / 2);
    }
    for (int i = 0; i < 6; i++) {
        Vector3 v0 = proj_3d[faces[i].v[0]]; Vector3 v1 = proj_3d[faces[i].v[1]]; Vector3 v2 = proj_3d[faces[i].v[2]];
        int dx1 = v1.x - v0.x; int dy1 = v1.y - v0.y; int dx2 = v2.x - v0.x; int dy2 = v2.y - v0.y;
        if ((dx1 * dy2) - (dy1 * dx2) < 0) continue;
        uint8_t col = (i + 1) * 40;
        Vertex2D sv0 = screen[faces[i].v[0]]; Vertex2D sv1 = screen[faces[i].v[1]];
        Vertex2D sv2 = screen[faces[i].v[2]]; Vertex2D sv3 = screen[faces[i].v[3]];
        draw_triangle(sv0, sv1, sv2, col); draw_triangle(sv0, sv2, sv3, col);
    }
    FILE *f = fopen("output.ppm", "w");
    fprintf(f, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    for(int i=0; i<WIDTH*HEIGHT; i++) fprintf(f, "%d %d %d ", backbuffer[i], backbuffer[i], backbuffer[i]);
    fclose(f);
    return 0;
}
