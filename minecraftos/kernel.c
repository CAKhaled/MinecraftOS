#include <stdint.h>
#include <stddef.h>
#include "textured_cube.h"
#include "os_config.h"
#include "CameraMove.h"
#include "CamMouseSensitivity.h"
#include "MapLoader.h"
#include "splash.h"
#include "canvas_ui.h"

// I/O ports
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

// PC Speaker functions
static void play_sound(uint32_t nFrequence) {
    uint32_t Div;
    uint8_t tmp;

    Div = 1193180 / nFrequence;
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t) (Div) );
    outb(0x42, (uint8_t) (Div >> 8));

    tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

static void nosound() {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

static void delay(uint32_t count) {
    while(count--) {
        asm volatile("nop");
    }
}

extern void update_background_music();

void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic;
    (void)addr;

    // تهيئة الشاشة أولاً (VGA Mode 13h)
    init_textured_renderer();
    g_camera_fov = STATIC_CAMERA_FOV;  // تطبيق FOV من config.txt

    // عرض شاشة الاقلاع
    show_splash_screen();

    play_sound(261); delay(15000000);
    play_sound(329); delay(15000000);
    play_sound(392); delay(15000000);
    nosound();

    init_mouse();
    init_camera();
    canvas_ui_init();

    // حلقة اللعبة الأساسية
    while(1) {
        // update_background_music(); // تم إيقاف الموسيقى بناءً على طلبك
        
        update_camera();
        
        begin_render((uint8_t)cam_angle_x);
        draw_map((uint8_t)cam_angle_x, (uint8_t)cam_angle_y, STATIC_ANGLE_Z, cam_x, cam_y, cam_z);
        canvas_ui_draw();   /* HUD فوق العالم ، ثابت لا يتأثر بالكاميرا */
        end_render();
        
        // تأخير بسيط لتقليل سرعة تحديث الشاشة
        delay(2000000);
    }
}
