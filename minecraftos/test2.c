#include <stdio.h>
#include <stdint.h>

#define WIDTH 320
#define HEIGHT 200
#define SCALE 256

uint8_t backbuffer[WIDTH * HEIGHT];
uint8_t tex_top_bottom[4096] = {1};
uint8_t tex_sides[4096] = {2};

static const int16_t sin_table[256] = {
    0, 6, 12, 18, 25, 31, 37, 43, 49, 56, 62, 68, 74, 80, 86, 92,
    97, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176,
    180, 185, 189, 193, 197, 201, 205, 208, 212, 215, 219, 222, 225, 228, 231, 233,
    236, 238, 240, 242, 244, 246, 247, 249, 250, 251, 252, 253, 254, 254, 255, 255,
    255, 255, 255, 254, 254, 253, 252, 251, 250, 249, 247, 246, 244, 242, 240, 238,
    236, 233, 231, 228, 225, 222, 219, 215, 212, 208, 205, 201, 197, 193, 189, 185,
    180, 176, 171, 167, 162, 157, 152, 147, 142, 136, 131, 126, 120, 115, 109, 103,
    97, 92, 86, 80, 74, 68, 62, 56, 49, 43, 37, 31, 25, 18, 12, 6,
    0, -6, -12, -18, -25, -31, -37, -43, -49, -56, -62, -68, -74, -80, -86, -92,
    -97, -103, -109, -115, -120, -126, -131, -136, -142, -147, -152, -157, -162, -167, -171, -176,
    -180, -185, -189, -193, -197, -201, -205, -208, -212, -215, -219, -222, -225, -228, -231, -233,
    -236, -238, -240, -242, -244, -246, -247, -249, -250, -251, -252, -253, -254, -254, -255, -255,
    -255, -255, -255, -254, -254, -253, -252, -251, -250, -249, -247, -246, -244, -242, -240, -238,
    -236, -233, -231, -228, -225, -222, -219, -215, -212, -208, -205, -201, -197, -193, -189, -185,
    -180, -176, -171, -167, -162, -157, -152, -147, -142, -136, -131, -126, -120, -115, -109, -103,
    -97, -92, -86, -80, -74, -68, -62, -56, -49, -43, -37, -31, -25, -18, -12, -6
};

static inline int16_t get_sin(uint8_t angle) { return sin_table[angle]; }
static inline int16_t get_cos(uint8_t angle) { return sin_table[(uint8_t)(angle + 64)]; }

typedef struct { int x, y, u, v; } Vertex2D;
typedef struct { int x, y, z; } Vector3;

static int edge_function(Vertex2D a, Vertex2D b, Vertex2D p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

static void draw_textured_triangle(Vertex2D v0, Vertex2D v1, Vertex2D v2, const uint8_t* tex) {
    int minX = v0.x; if (v1.x < minX) minX = v1.x; if (v2.x < minX) minX = v2.x;
    int minY = v0.y; if (v1.y < minY) minY = v1.y; if (v2.y < minY) minY = v2.y;
    int maxX = v0.x; if (v1.x > maxX) maxX = v1.x; if (v2.x > maxX) maxX = v2.x;
    int maxY = v0.y; if (v1.y > maxY) maxY = v1.y; if (v2.y > maxY) maxY = v2.y;
    if (minX < 0) minX = 0; if (minY < 0) minY = 0;
    if (maxX >= WIDTH) maxX = WIDTH - 1; if (maxY >= HEIGHT) maxY = HEIGHT - 1;

    int area = edge_function(v0, v1, v2);
    if (area == 0) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Vertex2D p = {x, y, 0, 0};
            int w0 = edge_function(v1, v2, p);
            int w1 = edge_function(v2, v0, p);
            int w2 = edge_function(v0, v1, p);
            
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                backbuffer[y * WIDTH + x] = 1;
            }
        }
    }
}

static const Vector3 cube_vertices[8] = {
    {-50, -50, -50}, { 50, -50, -50}, { 50,  50, -50}, {-50,  50, -50},
    {-50, -50,  50}, { 50, -50,  50}, { 50,  50,  50}, {-50,  50,  50}
};
typedef struct { int v[4]; int is_top_bottom; } Face;
static const Face faces[6] = {
    {{0, 1, 2, 3}, 0}, {{5, 4, 7, 6}, 0}, {{4, 5, 1, 0}, 1}, 
    {{3, 2, 6, 7}, 1}, {{4, 0, 3, 7}, 0}, {{1, 5, 6, 2}, 0}
};

void render_textured_cube(uint8_t angle_x, uint8_t angle_y, uint8_t angle_z) {
    int sinX = get_sin(angle_x), cosX = get_cos(angle_x);
    int sinY = get_sin(angle_y), cosY = get_cos(angle_y);
    int sinZ = get_sin(angle_z), cosZ = get_cos(angle_z);

    Vector3 proj_3d[8];
    Vertex2D screen[8];

    for (int i = 0; i < 8; i++) {
        int x = cube_vertices[i].x;
        int y = cube_vertices[i].y;
        int z = cube_vertices[i].z;

        int xy = (y * cosX - z * sinX) / SCALE;
        int xz = (y * sinX + z * cosX) / SCALE;
        y = xy; z = xz;

        int yx = (x * cosY + z * sinY) / SCALE;
        int yz = (-x * sinY + z * cosY) / SCALE;
        x = yx; z = yz;

        int zx = (x * cosZ - y * sinZ) / SCALE;
        int zy = (x * sinZ + y * cosZ) / SCALE;
        x = zx; y = zy;

        proj_3d[i].x = x; proj_3d[i].y = y; proj_3d[i].z = z;

        z += 150; 
        
        screen[i].x = (x * 200 / z) + (WIDTH / 2);
        screen[i].y = (y * 200 / z) + (HEIGHT / 2); 
    }

    for (int i = 0; i < 6; i++) {
        Vector3 v0 = proj_3d[faces[i].v[0]];
        Vector3 v1 = proj_3d[faces[i].v[1]];
        Vector3 v2 = proj_3d[faces[i].v[2]];
        
        int dx1 = v1.x - v0.x; int dy1 = v1.y - v0.y; int dz1 = v1.z - v0.z;
        int dx2 = v2.x - v0.x; int dy2 = v2.y - v0.y; int dz2 = v2.z - v0.z;
        
        int normalZ = (dx1 * dy2) - (dy1 * dx2);
        if (normalZ < 0) continue; 

        Vertex2D sv0 = screen[faces[i].v[0]]; sv0.u = 0;  sv0.v = 0;
        Vertex2D sv1 = screen[faces[i].v[1]]; sv1.u = 63; sv1.v = 0;
        Vertex2D sv2 = screen[faces[i].v[2]]; sv2.u = 63; sv2.v = 63;
        Vertex2D sv3 = screen[faces[i].v[3]]; sv3.u = 0;  sv3.v = 63;

        draw_textured_triangle(sv0, sv1, sv2, tex_top_bottom);
        draw_textured_triangle(sv0, sv2, sv3, tex_top_bottom);
    }
}

int main() {
    render_textured_cube(30, 45, 0);
    int pixels = 0;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (backbuffer[y * WIDTH + x] != 0) pixels++;
        }
    }
    printf("Pixels drawn: %d\n", pixels);
    return 0;
}
