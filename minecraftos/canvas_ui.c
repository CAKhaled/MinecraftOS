/*
 * canvas_ui.c — Canvas UI System (v2)
 *
 * الميزات:
 *   - Hotbar 9 خانات في الأسفل
 *   - Slot 0: Grass  |  Slot 1: Stone  |  Slots 2-8: فارغة
 *   - نص اسم البلوك فوق الـ Hotbar (لا تحته)
 *   - مفاتيح 1, 2, 3 لتحديد الخانة (PS/2 scancodes 0x02,0x03,0x04)
 *   - Crosshair في المركز
 *   - كل شيء ثابت لا يتأثر بالكاميرا
 */

#include "canvas_ui.h"
#include "CameraMove.h"
#include "textures.h"
#include <stdint.h>

/* ── أبعاد الشاشة ── */
#define SCR_W   320
#define SCR_H   200

/* ── الـ backbuffer ── */
extern uint8_t ui_backbuffer_ptr[SCR_W * SCR_H];

/* ── ASM helper ── */
extern void ui_fill_hline_asm(uint8_t *dest, uint8_t color, uint32_t pixels);

/* ── ألوان VGA Mode 13h ── */
#define C_BLACK   0
#define C_DKBLUE  1
#define C_DKGREEN 2
#define C_BROWN   6
#define C_GRAY    7
#define C_DKGRAY  8
#define C_GRGREEN 10  /* أخضر فاتح */
#define C_WHITE   15

/* ── إعدادات الـ Hotbar ── */
#define SLOT_W       18
#define SLOT_H       18
#define SLOT_GAP      2
#define NUM_SLOTS     9
#define HOTBAR_TOTAL_W (NUM_SLOTS * SLOT_W + (NUM_SLOTS - 1) * SLOT_GAP)
#define HOTBAR_X    ((SCR_W - HOTBAR_TOTAL_W) / 2)
#define HOTBAR_Y    (SCR_H - SLOT_H - 4)   /* أسفل الشاشة */

/* PS/2 Scancodes للأرقام */
#define KEY_1  0x02
#define KEY_2  0x03
#define KEY_3  0x04
#define KEY_4  0x05
#define KEY_5  0x06
#define KEY_6  0x07
#define KEY_7  0x08
#define KEY_8  0x09
#define KEY_9  0x0A

/* الخانة الحالية المحددة */
static int  g_selected = 0;
/* حالة المفاتيح السابقة (لاكتشاف ضغطة واحدة) */
static uint8_t g_prev_keys[9] = {0};

/* ═══════════════════════════════════════════════
   رسوميات الـ backbuffer
   ═══════════════════════════════════════════════ */

static inline void px(int x, int y, uint8_t c) {
    if ((unsigned)x < SCR_W && (unsigned)y < SCR_H)
        ui_backbuffer_ptr[y * SCR_W + x] = c;
}

static void fill(int x, int y, int w, int h, uint8_t c) {
    for (int row = y; row < y + h; row++) {
        if ((unsigned)row >= SCR_H) continue;
        int x0 = x < 0 ? 0 : x;
        int x1 = (x + w) > SCR_W ? SCR_W : (x + w);
        if (x0 >= x1) continue;
        ui_fill_hline_asm(&ui_backbuffer_ptr[row * SCR_W + x0], c,
                          (uint32_t)(x1 - x0));
    }
}

static void border(int x, int y, int w, int h, uint8_t c) {
    fill(x,     y,     w, 1, c);
    fill(x,     y+h-1, w, 1, c);
    fill(x,     y,     1, h, c);
    fill(x+w-1, y,     1, h, c);
}

/* ═══════════════════════════════════════════════
   Pixel Font 5×7
   الأحرف: G r a s S t o n e (لـ Grass/Stone)
   ═══════════════════════════════════════════════ */
typedef struct { char ch; uint8_t b[7]; } Glyph;

static const Glyph GLYPHS[] = {
    /* G  */ {'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}},
    /* r  */ {'r', {0x00,0x00,0x16,0x19,0x10,0x10,0x10}},
    /* a  */ {'a', {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}},
    /* s  */ {'s', {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E}},
    /* S  */ {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    /* t  */ {'t', {0x04,0x0E,0x04,0x04,0x04,0x04,0x03}},
    /* o  */ {'o', {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}},
    /* n  */ {'n', {0x00,0x00,0x16,0x19,0x11,0x11,0x11}},
    /* e  */ {'e', {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0F}},
    /* ' '*/ {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    /* sentinel */
    {0, {0}}
};

static const uint8_t *get_glyph(char c) {
    for (int i = 0; GLYPHS[i].ch; i++)
        if (GLYPHS[i].ch == c) return GLYPHS[i].b;
    return GLYPHS[9].b; /* space */
}

static void draw_char_ui(int x, int y, char c, uint8_t col) {
    const uint8_t *b = get_glyph(c);
    for (int row = 0; row < 7; row++)
        for (int col2 = 0; col2 < 5; col2++)
            if (b[row] & (1 << (4 - col2)))
                px(x + col2, y + row, col);
}

static int str_w(const char *s) {
    int n = 0; while (s[n]) n++;
    return n > 0 ? n * 6 - 1 : 0;
}

static void draw_str(int x, int y, const char *s, uint8_t col) {
    for (; *s; s++, x += 6)
        draw_char_ui(x, y, *s, col);
}

/* ═══════════════════════════════════════════════
   تعريفات البلوكات
   ═══════════════════════════════════════════════ */
typedef struct {
    const char *name;
    uint8_t     top_col;
    uint8_t     mid_col;
    uint8_t     bot_col;
    uint8_t     detail_col;
} BlockDef;

/* مصفوفة البلوكات — null name = خانة فارغة */
static const BlockDef BLOCKS[NUM_SLOTS] = {
    /* 0 - Grass */  {"Grass", C_GRGREEN, C_GRGREEN, C_BROWN,  C_DKGREEN},
    /* 1 - Stone */  {"Stone", C_GRAY,    C_GRAY,    C_GRAY,   C_DKGRAY},
    /* 2..8 empty */ {0},
    {0},{0},{0},{0},{0},{0}
};

/* ── رسم أيقونة 12×12 للبلوك ── */
static void draw_block_icon(int ox, int oy, int slot) {
    if (slot < 0 || slot >= NUM_SLOTS) return;
    const BlockDef *b = &BLOCKS[slot];
    if (!b->name) return;

    if (slot == 0) {
        // Draw Grass UI image
        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 12; x++) {
                uint8_t color = tex_ui_grass[y * 12 + x];
                px(ox + x, oy + y, color);
            }
        }
    } else if (slot == 1) {
        // Draw Stone UI image
        for (int y = 0; y < 12; y++) {
            for (int x = 0; x < 12; x++) {
                uint8_t color = tex_ui_stone[y * 12 + x];
                px(ox + x, oy + y, color);
            }
        }
    } else {
        if (b->top_col != b->bot_col) {
            /* بلوك بطبقتين (مثل Grass) */
            fill(ox, oy,     12, 5, b->top_col);
            /* تفاصيل القمة */
            px(ox+1, oy+1, b->detail_col);
            px(ox+4, oy+0, b->detail_col);
            px(ox+7, oy+2, b->detail_col);
            px(ox+10,oy+1, b->detail_col);
            px(ox+3, oy+3, b->detail_col);
            px(ox+8, oy+4, b->detail_col);
            fill(ox, oy+5,  12, 7, b->bot_col);
            /* تفاصيل القاعدة */
            px(ox+2, oy+6,  b->detail_col);
            px(ox+6, oy+8,  b->detail_col);
            px(ox+9, oy+7,  b->detail_col);
            px(ox+4, oy+10, b->detail_col);
            px(ox+11,oy+9,  b->detail_col);
        } else {
            /* بلوك موحد (مثل Stone) */
            fill(ox, oy, 12, 12, b->top_col);
            /* تفاصيل Stone */
            px(ox+2, oy+2,  b->detail_col);
            px(ox+5, oy+1,  b->detail_col);
            px(ox+9, oy+3,  b->detail_col);
            px(ox+3, oy+6,  b->detail_col);
            px(ox+7, oy+5,  b->detail_col);
            px(ox+1, oy+9,  b->detail_col);
            px(ox+8, oy+9,  b->detail_col);
            px(ox+10,oy+7,  b->detail_col);
            /* حدود داكنة */
            border(ox, oy, 12, 12, b->detail_col);
        }
    }
}

/* ═══════════════════════════════════════════════
   Crosshair
   ═══════════════════════════════════════════════ */
static void draw_crosshair(void) {
    int cx = SCR_W / 2, cy = SCR_H / 2;
    fill(cx - 6, cy,     5, 1, C_WHITE);
    fill(cx + 2, cy,     5, 1, C_WHITE);
    fill(cx,     cy - 6, 1, 5, C_WHITE);
    fill(cx,     cy + 2, 1, 5, C_WHITE);
}

/* ═══════════════════════════════════════════════
   Hotbar + Label فوقه
   ═══════════════════════════════════════════════ */
static void draw_hotbar(void) {
    /* خلفية الشريط */
    int bx = HOTBAR_X - 2, by = HOTBAR_Y - 2;
    int bw = HOTBAR_TOTAL_W + 4, bh = SLOT_H + 4;
    fill(bx, by, bw, bh, C_DKGRAY);
    border(bx, by, bw, bh, C_GRAY);

    /* الخانات */
    for (int i = 0; i < NUM_SLOTS; i++) {
        int sx = HOTBAR_X + i * (SLOT_W + SLOT_GAP);
        int sy = HOTBAR_Y;

        fill(sx, sy, SLOT_W, SLOT_H, C_BLACK);

        if (i == g_selected) {
            border(sx - 1, sy - 1, SLOT_W + 2, SLOT_H + 2, C_WHITE);
            border(sx,     sy,     SLOT_W,     SLOT_H,     C_GRAY);
        } else {
            border(sx, sy, SLOT_W, SLOT_H, C_DKGRAY);
        }

        /* أيقونة البلوك */
        if (BLOCKS[i].name) {
            int ix = sx + (SLOT_W - 12) / 2;
            int iy = sy + (SLOT_H - 12) / 2;
            draw_block_icon(ix, iy, i);
        }
    }

    /* ── نص اسم البلوك فوق الـ Hotbar ── */
    const char *label = BLOCKS[g_selected].name;
    if (!label) label = "";
    int lw = str_w(label);

    /* مركز الخانة المحددة أفقياً */
    int sel_cx = HOTBAR_X + g_selected * (SLOT_W + SLOT_GAP) + SLOT_W / 2;
    int lx = sel_cx - lw / 2;
    int ly = HOTBAR_Y - 7 - 3; /* فوق الشريط بـ 10 بكسل */

    /* خلفية صغيرة للنص */
    fill(lx - 2, ly - 1, lw + 4, 9, C_DKGRAY);
    /* ظل */
    draw_str(lx + 1, ly + 1, label, C_BLACK);
    /* النص */
    draw_str(lx,     ly,     label, C_WHITE);
}

/* ═══════════════════════════════════════════════
   معالجة المفاتيح 1-9
   ═══════════════════════════════════════════════ */
static const uint8_t SLOT_KEYS[NUM_SLOTS] = {
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,
    KEY_6, KEY_7, KEY_8, KEY_9
};

static void handle_slot_keys(void) {
    for (int i = 0; i < NUM_SLOTS; i++) {
        uint8_t cur = (uint8_t)get_key_state(SLOT_KEYS[i]);
        /* اكتشاف leading edge (0→1) فقط */
        if (cur && !g_prev_keys[i]) {
            g_selected = i;
        }
        g_prev_keys[i] = cur;
    }
}

/* ═══════════════════════════════════════════════
   الدوال العامة
   ═══════════════════════════════════════════════ */
void canvas_ui_init(void) {
    g_selected = 0;
    for (int i = 0; i < NUM_SLOTS; i++) g_prev_keys[i] = 0;
}

void canvas_ui_draw(void) {
    handle_slot_keys();
    draw_crosshair();
    draw_hotbar();
}

void canvas_ui_set_slot(int slot) {
    if (slot >= 0 && slot < NUM_SLOTS)
        g_selected = slot;
}
int canvas_ui_get_selected(void) {
    return g_selected;
}
