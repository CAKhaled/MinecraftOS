#include "textured_cube.h"
#include "textures.h" // يحتوي على الصور المحولة tex_top_bottom و tex_sides
#include "canvas_ui.h"

#define WIDTH 320
#define HEIGHT 200
#define SCALE 256

/* FOV scale factor – set at startup by kernel.c via init_camera_fov() */
int g_camera_fov = 256;

// الذاكرة المؤقتة لمنع الوميض (Double Buffering)
// مُعرَّفة بشكل يسمح لنظام الـ UI بالوصول المباشر
uint8_t ui_backbuffer_ptr[WIDTH * HEIGHT];
#define backbuffer ui_backbuffer_ptr

// I/O ports
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

// مصفوفة كرت الشاشة لوضع VGA Mode 13h
static unsigned char g_320x200x256[] = {
    0x63, 0x03, 0x01, 0x0F, 0x00, 0x0E,
    0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
    0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x41, 0x00, 0x0F, 0x00, 0x00
};

static void write_regs(unsigned char *regs) {
    unsigned i;
    outb(0x3C2, *regs); regs++;
    for (i = 0; i < 5; i++) { outb(0x3C4, i); outb(0x3C5, *regs); regs++; }
    outb(0x3D4, 0x03); outb(0x3D5, inb(0x3D5) | 0x80);
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & ~0x80);
    regs[0x03] |= 0x80; regs[0x11] &= ~0x80;
    for (i = 0; i < 25; i++) { outb(0x3D4, i); outb(0x3D5, *regs); regs++; }
    for (i = 0; i < 9; i++) { outb(0x3CE, i); outb(0x3CF, *regs); regs++; }
    for (i = 0; i < 21; i++) { inb(0x3DA); outb(0x3C0, i); outb(0x3C0, *regs); regs++; }
    inb(0x3DA); outb(0x3C0, 0x20);
}

static void set_vga_palette() {
    outb(0x3C8, 0); // Start at color index 0
    for(int i = 0; i < 256 * 3; i++) {
        // VGA DAC accepts values from 0 to 63, but our array is 0-255
        outb(0x3C9, vga_palette_data[i] / 4); 
    }
}

#include "math3d.h"

// Sine table
const int16_t sin_table[256] = {
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


void clear_screen_textured(uint8_t pitch) {
    int horizon = (HEIGHT / 2) - (get_sin(pitch) * (HEIGHT / 2)) / 256;
    if (horizon < 0) horizon = 0;
    if (horizon > HEIGHT) horizon = HEIGHT;

    for(int y = 0; y < HEIGHT; y++) {
        uint8_t color = 0;
        if (y < horizon) {
            color = 59; // Sky Blue
        } else {
            color = 59; // Same Sky Blue for ground void
        }
        for(int x = 0; x < WIDTH; x++) {
            backbuffer[y * WIDTH + x] = color;
        }
    }
}

void init_textured_renderer() {
    write_regs(g_320x200x256);
    set_vga_palette();
    clear_screen_textured(0);
}

static void flush_buffer() {
    volatile uint8_t* vga = (volatile uint8_t*)0xA0000;
    for(int i = 0; i < WIDTH * HEIGHT; i++) {
        vga[i] = backbuffer[i];
    }
}

/* ─── واجهة الـ UI ─────────────────────────────────── */
void ui_put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        backbuffer[y * WIDTH + x] = color;
}
void ui_fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int ry = y; ry < y + h; ry++)
        for (int rx = x; rx < x + w; rx++)
            ui_put_pixel(rx, ry, color);
}


static int abs(int x) { return x < 0 ? -x : x; }

static void draw_pixel_textured(int x, int y, uint8_t color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        backbuffer[y * WIDTH + x] = color;
        if (x + 1 < WIDTH) backbuffer[y * WIDTH + x + 1] = color;
        if (y + 1 < HEIGHT) backbuffer[(y + 1) * WIDTH + x] = color;
        if (x + 1 < WIDTH && y + 1 < HEIGHT) backbuffer[(y + 1) * WIDTH + x + 1] = color;
    }
}

static void draw_line_textured(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        draw_pixel_textured(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

typedef struct { int x, y, u, v; } Vertex2D;
typedef struct { int x, y, z; } Vector3;

// ─── Near Plane Clipping (Sutherland-Hodgman) ───────────────────────────────
#define NEAR_PLANE 5

typedef struct { int x, y, z, u, v; } Vertex3D;

static const int face_uvs[4][2] = {{0,0},{63,0},{63,63},{0,63}};

static int clip_polygon_near(Vertex3D* in, int in_count, Vertex3D* out) {
    int out_count = 0;
    for (int i = 0; i < in_count; i++) {
        Vertex3D curr = in[i];
        Vertex3D next = in[(i + 1) % in_count];
        int curr_in = (curr.z >= NEAR_PLANE);
        int next_in = (next.z >= NEAR_PLANE);
        if (curr_in) {
            out[out_count++] = curr;
        }
        if (curr_in != next_in) {
            int dz    = next.z - curr.z;     // never 0 here
            int t_num = NEAR_PLANE - curr.z; // numerator of t
            Vertex3D p;
            p.z = NEAR_PLANE;
            p.x = curr.x + (t_num * (next.x - curr.x)) / dz;
            p.y = curr.y + (t_num * (next.y - curr.y)) / dz;
            p.u = curr.u + (t_num * (next.u - curr.u)) / dz;
            p.v = curr.v + (t_num * (next.v - curr.v)) / dz;
            out[out_count++] = p;
        }
    }
    return out_count;
}
// ─────────────────────────────────────────────────────────────────────────────

// إيجاد المساحة لتحديد الإضاءة الخلفية (Backface culling) والتلوين
static int edge_function(Vertex2D a, Vertex2D b, Vertex2D p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

// رسم مثلث مكسو بالصورة
static void draw_textured_triangle(Vertex2D v0, Vertex2D v1, Vertex2D v2, const uint8_t* tex) {
    int minX = v0.x; if (v1.x < minX) minX = v1.x; if (v2.x < minX) minX = v2.x;
    int minY = v0.y; if (v1.y < minY) minY = v1.y; if (v2.y < minY) minY = v2.y;
    int maxX = v0.x; if (v1.x > maxX) maxX = v1.x; if (v2.x > maxX) maxX = v2.x;
    int maxY = v0.y; if (v1.y > maxY) maxY = v1.y; if (v2.y > maxY) maxY = v2.y;

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= WIDTH) maxX = WIDTH - 1;
    if (maxY >= HEIGHT) maxY = HEIGHT - 1;

    int area = edge_function(v0, v1, v2);
    if (area == 0) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Vertex2D p = {x, y, 0, 0};
            int w0 = edge_function(v1, v2, p);
            int w1 = edge_function(v2, v0, p);
            int w2 = edge_function(v0, v1, p);

            // السماح برسم المثلث بغض النظر عن اتجاه ترتيب الرؤوس (Clockwise أو Counter-Clockwise)
            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                int u = (w0 * v0.u + w1 * v1.u + w2 * v2.u) / area;
                int v = (w0 * v0.v + w1 * v1.v + w2 * v2.v) / area;
                
                if (u < 0) u = 0; 
                if (u > 63) u = 63;
                if (v < 0) v = 0; 
                if (v > 63) v = 63;

                backbuffer[y * WIDTH + x] = tex[v * 64 + u];
            }
        }
    }
}

// رؤوس المكعب 3D (تم تكبيرها قليلاً لـ 51 لتغطي أي فراغات بصرية بين المكعبات)
static const Vector3 cube_vertices[8] = {
    {-51, -51, -51}, { 51, -51, -51}, { 51,  51, -51}, {-51,  51, -51}, // 0,1,2,3 (Front)
    {-51, -51,  51}, { 51, -51,  51}, { 51,  51,  51}, {-51,  51,  51}  // 4,5,6,7 (Back)
};

// تعريف الأوجه (كل وجه يتكون من 4 رؤوس)
typedef struct { int v[4]; int is_top_bottom; } Face;

static const Face faces[6] = {
    {{0, 1, 2, 3}, 0}, // Front
    {{5, 4, 7, 6}, 0}, // Back
    {{4, 5, 1, 0}, 1}, // Top 
    {{3, 2, 6, 7}, 1}, // Bottom
    {{4, 0, 3, 7}, 0}, // Left
    {{1, 5, 6, 2}, 0}  // Right
};

void begin_render(uint8_t pitch) {
    clear_screen_textured(pitch);
}

void end_render() {
    flush_buffer();
}

void render_cube_at(int world_x, int world_y, int world_z, 
                    uint8_t angle_x, uint8_t angle_y, uint8_t angle_z, 
                    int cam_x, int cam_y, int cam_z, int is_hovered,
                    uint8_t block_type) {
    int sinX = get_sin(angle_x), cosX = get_cos(angle_x);
    int sinY = get_sin(angle_y), cosY = get_cos(angle_y);
    int sinZ = get_sin(angle_z), cosZ = get_cos(angle_z);

    // ── Step 1: Transform all 8 vertices to camera space (3D, no projection yet) ──
    Vector3 proj_3d[8];

    for (int i = 0; i < 8; i++) {
        long x = cube_vertices[i].x;
        long y = cube_vertices[i].y;
        long z = cube_vertices[i].z;

        // World Translation
        x += world_x; y += world_y; z += world_z;
        // Camera Translation
        x -= cam_x;   y -= cam_y;   z -= cam_z;

        // Yaw (Y rotation) first
        int yx = (x * cosY + z * sinY) / 256;
        int yz = (-x * sinY + z * cosY) / 256;
        x = yx; z = yz;

        // Pitch (X rotation) second
        int xy = (y * cosX - z * sinX) / 256;
        int xz = (y * sinX + z * cosX) / 256;
        y = xy; z = xz;

        // Roll (Z rotation) — usually zero
        int zx = (x * cosZ - y * sinZ) / 256;
        int zy = (x * sinZ + y * cosZ) / 256;
        x = zx; y = zy;

        proj_3d[i].x = (int)x;
        proj_3d[i].y = (int)y;
        proj_3d[i].z = (int)z;
        // NO clamping and NO projection here — clipping handles it correctly
    }

    // ── Step 2-6: Per-face near-plane clipping → projection → culling → draw ──
    for (int i = 0; i < 6; i++) {

        // Build a Vertex3D polygon from the camera-space vertices + assign UVs
        Vertex3D cam_verts[4];
        for (int j = 0; j < 4; j++) {
            cam_verts[j].x = proj_3d[faces[i].v[j]].x;
            cam_verts[j].y = proj_3d[faces[i].v[j]].y;
            cam_verts[j].z = proj_3d[faces[i].v[j]].z;
            cam_verts[j].u = face_uvs[j][0];
            cam_verts[j].v = face_uvs[j][1];
        }

        // Sutherland-Hodgman clip against near plane
        // Result can be 0-5 vertices (quad clipped by one plane = at most 5 verts)
        Vertex3D clipped[8];
        int clipped_count = clip_polygon_near(cam_verts, 4, clipped);
        if (clipped_count < 3) continue; // Entirely behind camera

        // Perspective-project the clipped vertices using g_camera_fov as focal length
        Vertex2D sv[8];
        for (int j = 0; j < clipped_count; j++) {
            sv[j].x = (clipped[j].x * g_camera_fov) / clipped[j].z + (WIDTH  / 2);
            sv[j].y = (clipped[j].y * g_camera_fov) / clipped[j].z + (HEIGHT / 2);
            sv[j].u = clipped[j].u;
            sv[j].v = clipped[j].v;
        }

        // Backface culling on projected vertices (winding is preserved by S-H)
        int dx1 = sv[1].x - sv[0].x, dy1 = sv[1].y - sv[0].y;
        int dx2 = sv[2].x - sv[0].x, dy2 = sv[2].y - sv[0].y;
        int normalZ = (dx1 * dy2) - (dy1 * dx2);
        if (normalZ <= 0) continue;

        // Draw as triangle fan
        const uint8_t *tex;
        if (faces[i].is_top_bottom) {
            tex = (block_type == 2) ? tex_top_bottom_stone : tex_top_bottom;
        } else {
            tex = (block_type == 2) ? tex_sides_stone : tex_sides;
        }
        for (int j = 1; j < clipped_count - 1; j++) {
            draw_textured_triangle(sv[0], sv[j], sv[j + 1], tex);
        }

        // Wireframe highlight (drawn on the clipped polygon edges)
        if (is_hovered) {
            for (int j = 0; j < clipped_count; j++) {
                int nxt = (j + 1) % clipped_count;
                draw_line_textured(sv[j].x, sv[j].y, sv[nxt].x, sv[nxt].y, 15);
            }
        }
    }
}
