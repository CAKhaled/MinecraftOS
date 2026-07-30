#include "splash.h"
#include <stdint.h>

#define WIDTH  320
#define HEIGHT 200

/* ألوان VGA Mode 13h */
#define C_BLACK    0
#define C_DKRED    4
#define C_DKGRAY   8
#define C_BRTRED   12
#define C_WHITE    15

/* ===== رسم بكسل ===== */
static inline void put_pixel(int x, int y, uint8_t color) {
    if ((unsigned)x < WIDTH && (unsigned)y < HEIGHT)
        ((uint8_t *)0xA0000)[y * WIDTH + x] = color;
}

static void fill_rect(int x, int y, int w, int h, uint8_t col) {
    for (int ry = y; ry < y + h; ry++)
        for (int rx = x; rx < x + w; rx++)
            put_pixel(rx, ry, col);
}

static void splash_delay(uint32_t n) {
    while (n--) asm volatile("nop");
}

/* ===== خط بكسل بدون حساب float ===== */
static void draw_hline(int x0, int x1, int y, uint8_t col) {
    for (int x = x0; x <= x1; x++) put_pixel(x, y, col);
}
static void draw_vline(int x, int y0, int y1, uint8_t col) {
    for (int y = y0; y <= y1; y++) put_pixel(x, y, col);
}


/* ===== Bitmap font 5×7 ===== */
/* كل صف = 5 bits من MSB */
static const uint8_t FONT[][7] = {
    /* 0  A */ {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* 1  B */ {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    /* 2  D */ {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    /* 3  E */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    /* 4  H */ {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* 5  K */ {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    /* 6  L */ {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    /* 7  N */ {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    /* 8  O */ {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* 9  P */ {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    /* 10 R */ {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    /* 11 V */ {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04},
    /* 12 Y */ {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    /* 13 Z */ {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    /* 14 SPC*/ {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 15 d */ {0x01,0x01,0x0D,0x13,0x11,0x13,0x0D},
    /* 16 e */ {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0F},
    /* 17 v */ {0x00,0x00,0x11,0x11,0x11,0x0A,0x04},
    /* 18 o */ {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},
    /* 19 p */ {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10},
    /* 20 l */ {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
    /* 21 a */ {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F},
    /* 22 y */ {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E},
    /* 23 h */ {0x10,0x10,0x16,0x19,0x11,0x11,0x11},
    /* 24 b */ {0x10,0x10,0x16,0x19,0x11,0x19,0x16},
    /* 25 i */ {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E},
    /* 26 n */ {0x00,0x00,0x16,0x19,0x11,0x11,0x11},
    /* 27 r */ {0x00,0x00,0x16,0x19,0x10,0x10,0x10},
    /* 28 g */ {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E},
};

typedef struct { char c; uint8_t i; } CM;
static const CM CMAP[] = {
    {'A',0},{'B',1},{'D',2},{'E',3},{'H',4},{'K',5},{'L',6},
    {'N',7},{'O',8},{'P',9},{'R',10},{'V',11},{'Y',12},{'Z',13},
    {' ',14},
    {'d',15},{'e',16},{'v',17},{'o',18},{'p',19},{'l',20},
    {'a',21},{'y',22},{'h',23},{'b',24},{'i',25},{'n',26},
    {'r',27},{'g',28},
    {0,14}
};

static uint8_t fidx(char c){
    for(int i=0;CMAP[i].c;i++) if(CMAP[i].c==c) return CMAP[i].i;
    return 14;
}

static void draw_char(char c, int px, int py, uint8_t col, int sc){
    uint8_t fi=fidx(c);
    for(int row=0;row<7;row++){
        uint8_t b=FONT[fi][row];
        for(int col2=0;col2<5;col2++){
            if(b&(1<<(4-col2)))
                for(int sy=0;sy<sc;sy++)
                    for(int sx=0;sx<sc;sx++)
                        put_pixel(px+col2*sc+sx,py+row*sc+sy,col);
        }
    }
}

static int str_len(const char*s){int n=0;while(s[n])n++;return n;}

static void draw_text(const char*s,int px,int py,uint8_t col,int sc){
    for(int i=0;s[i];i++,px+=(5+1)*sc)
        draw_char(s[i],px,py,col,sc);
}

static int text_w(const char*s,int sc){
    int n=str_len(s);
    return n*(5+1)*sc-sc;
}

/* ===== رسم إطار مزخرف بزوايا ===== */
static void draw_frame(int x0,int y0,int x1,int y1,uint8_t outer,uint8_t inner){
    /* الخطوط الخارجية */
    draw_hline(x0,x1,y0,outer);
    draw_hline(x0,x1,y1,outer);
    draw_vline(x0,y0,y1,outer);
    draw_vline(x1,y0,y1,outer);
    /* خطوط داخلية */
    draw_hline(x0+2,x1-2,y0+2,inner);
    draw_hline(x0+2,x1-2,y1-2,inner);
    draw_vline(x0+2,y0+2,y1-2,inner);
    draw_vline(x1-2,y0+2,y1-2,inner);
    /* نقاط الزوايا */
    fill_rect(x0,y0,2,2,inner);
    fill_rect(x1-1,y0,2,2,inner);
    fill_rect(x0,y1-1,2,2,inner);
    fill_rect(x1-1,y1-1,2,2,inner);
}

/* ===== نجوم عشوائية في الخلفية ===== */
static void draw_stars(){
    /* مواقع ثابتة محسوبة مسبقاً — x في نطاق 0..319، y في نطاق 0..66 */
    static const uint16_t sx[]={10,30,55,90,130,170,210,250,285,315,
                                  20,60,100,140,180,220,255,298,15,80,
                                  120,160,200,240,275,300,45,75,115,195};
    static const uint8_t  sy[]={10,25,15, 8, 20, 12, 18, 9, 22, 14,
                                  35,40,30,38, 33, 42, 28,36, 50, 45,
                                  55,48,52,47, 58, 44, 62,38, 66, 60};
    for(int i=0;i<30;i++)
        put_pixel((int)sx[i], sy[i], C_DKGRAY);
    for(int i=0;i<30;i++)
        put_pixel((int)sx[i], HEIGHT-sy[i]-1, C_DKGRAY);
}

/* ===== الدالة الرئيسية ===== */
void show_splash_screen(){

    /* — خلفية سوداء كاملة — */
    fill_rect(0,0,WIDTH,HEIGHT,C_BLACK);
    draw_stars();

    /* ثوابت النص */
    const char *line1 = "Developed By";
    const char *line2 = "Khaled AlZahrani";
    int sc1=2, sc2=2;
    int w1=text_w(line1,sc1);
    int w2=text_w(line2,sc2);
    int x1=(WIDTH-w1)/2, x2=(WIDTH-w2)/2;
    int y1=HEIGHT/2-26, y2=HEIGHT/2+2;
    int h1=7*sc1, h2=7*sc2;

    /* حدود الإطار */
    int fx=x2-10, fy=y1-10;
    int fw=w2+20, fh=h1+h2+32;

    /* —— مرحلة 1: رسم الإطار من الوسط للأطراف —— */
    for(int step=0;step<=fw/2;step+=4){
        draw_hline(fx+fw/2-step, fx+fw/2+step, fy,    C_DKRED);
        draw_hline(fx+fw/2-step, fx+fw/2+step, fy+fh, C_DKRED);
        int vstep=step*fh/fw;
        draw_vline(fx,    fy+fh/2-vstep, fy+fh/2+vstep, C_DKRED);
        draw_vline(fx+fw, fy+fh/2-vstep, fy+fh/2+vstep, C_DKRED);
        splash_delay(2000000);  /* تأخير كافٍ لكل خطوة */
    }
    draw_frame(fx,fy,fx+fw,fy+fh,C_BRTRED,C_DKRED);
    draw_hline(fx+6, fx+fw-6, y1+h1+5, C_DKRED);

    /* —— مرحلة 2: "Developed By" مع flash —— */
    splash_delay(5000000);
    draw_text(line1, x1, y1, C_DKRED, sc1);
    splash_delay(8000000);
    draw_text(line1, x1, y1, C_WHITE, sc1);
    splash_delay(5000000);
    draw_text(line1, x1, y1, C_BRTRED, sc1);

    /* —— مرحلة 3: "Khaled AlZahrani" حرفاً حرفاً —— */
    splash_delay(5000000);
    int cx=x2;
    for(int i=0;line2[i];i++){
        draw_char(line2[i], cx, y2, C_WHITE, sc2);
        splash_delay(3000000);
        draw_char(line2[i], cx, y2, C_BRTRED, sc2);
        cx+=(5+1)*sc2;
    }

    /* —— مرحلة 4: وقفة طويلة ~3 ثوانٍ —— */
    /* 0xFFFFFFFF = 4.3 مليار nop ≈ 1-2 ثانية على الجهاز */
    splash_delay(0xFFFFFFFF);
    splash_delay(0xFFFFFFFF);
    splash_delay(0xFFFFFFFF);

    /* —— مرحلة 5: وميض بسيط —— */
    draw_text(line1,x1,y1,C_BLACK,sc1);
    draw_text(line2,x2,y2,C_BLACK,sc2);
    splash_delay(0x5FFFFFFF);
    draw_text(line1,x1,y1,C_BRTRED,sc1);
    draw_text(line2,x2,y2,C_BRTRED,sc2);
    splash_delay(0x5FFFFFFF);

    /* —— مرحلة 6: خروج — مسح الشاشة —— */
    fill_rect(0,0,WIDTH,HEIGHT,C_BLACK);
}
