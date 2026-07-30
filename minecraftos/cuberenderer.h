#ifndef CUBERENDERER_H
#define CUBERENDERER_H

#include <stdint.h>

// يهيئ الشاشة ويمسحها
void init_renderer();

// يمسح الشاشة فقط
void clear_screen();

void draw_line(int x0, int y0, int x1, int y1, uint8_t color);

void init_textured_renderer();
void begin_render();
void render_cube_at(int world_x, int world_y, int world_z, 
                    uint8_t angle_x, uint8_t angle_y, uint8_t angle_z, 
                    int cam_x, int cam_y, int cam_z, int is_hovered,
                    uint8_t block_type);
void end_render();

// يعرض الرسم النهائي على الشاشة (VGA Memory)
void flush_renderer();

#endif
