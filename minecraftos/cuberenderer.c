#include "cuberenderer.h"

#define WIDTH 320
#define HEIGHT 200
#define SCALE 256

// I/O ports لمخاطبة كرت الشاشة
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

// مصفوفة إعدادات كرت الشاشة VGA للانتقال إلى Mode 13h (320x200 بدقة 256 لون)
static unsigned char g_320x200x256[] = {
    /* MISC */ 0x63,
    /* SEQ */  0x03, 0x01, 0x0F, 0x00, 0x0E,
    /* CRTC */ 0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
               0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
               0xFF,
    /* GC */   0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
               0xFF,
    /* AC */   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
               0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
               0x41, 0x00, 0x0F, 0x00, 0x00
};

// دالة كتابة الإعدادات لكرت الشاشة
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

void clear_screen() {
    uint8_t* vga = (uint8_t*)0xA0000;
    for(int i = 0; i < WIDTH * HEIGHT; i++) {
        vga[i] = 0; // اللون 0 هو الأسود
    }
}

void init_renderer() {
    // تفعيل وضع الرسوميات الحقيقي (VGA Graphics Mode 13h)
    write_regs(g_320x200x256);
    clear_screen();
}

// دالة رسم "Pixel" حقيقي على الشاشة
void draw_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        uint8_t* vga = (uint8_t*)0xA0000;
        vga[y * WIDTH + x] = color;
    }
}

static int abs(int x) { return x < 0 ? -x : x; }

// دالة رسم خط (Line) باستخدام خوارزمية Bresenham للبيكسلات
void draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

typedef struct { int x, y, z; } Vector3;

static const Vector3 cube_vertices[8] = {
    {-50, -50, -50}, { 50, -50, -50}, { 50,  50, -50}, {-50,  50, -50},
    {-50, -50,  50}, { 50, -50,  50}, { 50,  50,  50}, {-50,  50,  50}
};

static const int edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

void render_cube(uint8_t angle_x, uint8_t angle_y, uint8_t angle_z) {
    clear_screen(); // يمسح الشاشة فقط بدون إعادة تهيئة الكرت

    int sinX = get_sin(angle_x), cosX = get_cos(angle_x);
    int sinY = get_sin(angle_y), cosY = get_cos(angle_y);
    int sinZ = get_sin(angle_z), cosZ = get_cos(angle_z);

    Vector3 proj[8];

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

        z += 150; 
        
        // إسقاط حقيقي (320x200)
        proj[i].x = (x * 200 / z) + (WIDTH / 2);
        proj[i].y = (y * 200 / z) + (HEIGHT / 2); 
    }

    // رسم الخطوط الفعلية باستخدام لون 10 (أخضر فاتح)
    for (int i = 0; i < 12; i++) {
        draw_line(proj[edges[i][0]].x, proj[edges[i][0]].y,
                  proj[edges[i][1]].x, proj[edges[i][1]].y, 10);
    }
    
    // يمكنك أيضا رسم نقاط مميزة في الرؤوس بلون مختلف (مثلا أحمر = 4)
    for (int i = 0; i < 8; i++) {
        draw_pixel(proj[i].x, proj[i].y, 4);
        draw_pixel(proj[i].x+1, proj[i].y, 4);
        draw_pixel(proj[i].x, proj[i].y+1, 4);
        draw_pixel(proj[i].x+1, proj[i].y+1, 4);
    }
}

void rotatecube(uint8_t x, uint8_t y, uint8_t z) {
    // ترسم المكعب بثبات بالزوايا المحددة
    render_cube(x, y, z);
}

void flush_renderer() {
    // لم نعد بحاجة لهذه الدالة لأن الرسم يتم مباشرة في ذاكرة الشاشة
}
