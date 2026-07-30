#ifndef TEXTURED_CUBE_H
#define TEXTURED_CUBE_H

#include <stdint.h>

void init_textured_renderer();
void begin_render(uint8_t pitch);
void render_cube_at(int world_x, int world_y, int world_z, 
                    uint8_t angle_x, uint8_t angle_y, uint8_t angle_z, 
                    int cam_x, int cam_y, int cam_z, int is_hovered,
                    uint8_t block_type);
void end_render();
void clear_screen_textured(uint8_t pitch);

/* واجهة الـ UI ─ رسم مباشرة في الـ backbuffer */
void ui_put_pixel(int x, int y, uint8_t color);
void ui_fill_rect(int x, int y, int w, int h, uint8_t color);

/* FOV – القيمة الافتراضية 256 تعادل ~90 درجة
 * 128 = تكبير (ضيّق) | 512 = تصغير (واسع)           */
extern int g_camera_fov;

#endif
