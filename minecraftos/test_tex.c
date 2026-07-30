#include <stdio.h>
#include <stdint.h>

#define WIDTH 320
#define HEIGHT 200

uint8_t backbuffer[WIDTH * HEIGHT];
uint8_t tex_sides[4096];

typedef struct { int x, y, u, v; } Vertex2D;

static int edge_function(Vertex2D a, Vertex2D b, Vertex2D p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

static void draw_textured_triangle(Vertex2D v0, Vertex2D v1, Vertex2D v2, const uint8_t* tex) {
    int minX = v0.x; if (v1.x < minX) minX = v1.x; if (v2.x < minX) minX = v2.x;
    int minY = v0.y; if (v1.y < minY) minY = v1.y; if (v2.y < minY) minY = v2.y;
    int maxX = v0.x; if (v1.x > maxX) maxX = v1.x; if (v2.x > maxX) maxX = v2.x;
    int maxY = v0.y; if (v1.y > maxY) maxY = v1.y; if (v2.y > maxY) maxY = v2.y;

    int area = edge_function(v0, v1, v2);
    if (area == 0) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Vertex2D p = {x, y, 0, 0};
            int w0 = edge_function(v1, v2, p);
            int w1 = edge_function(v2, v0, p);
            int w2 = edge_function(v0, v1, p);
            
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                int u = (w0 * v0.u + w1 * v1.u + w2 * v2.u) / area;
                int v = (w0 * v0.v + w1 * v1.v + w2 * v2.v) / area;
                backbuffer[y * WIDTH + x] = tex[v * 64 + u];
            }
        }
    }
}

int main() {
    for(int i=0; i<4096; i++) tex_sides[i] = i % 256;
    Vertex2D v0 = {100, 100, 0, 0};
    Vertex2D v1 = {200, 100, 63, 0};
    Vertex2D v2 = {150, 150, 31, 63};
    draw_textured_triangle(v0, v1, v2, tex_sides);
    
    for (int y = 100; y < 105; y++) {
        for (int x = 145; x < 155; x++) {
            printf("%3d ", backbuffer[y * WIDTH + x]);
        }
        printf("\n");
    }
    return 0;
}
