#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <stdint.h>

#define MAP_WIDTH 40
#define MAP_DEPTH 40
#define MAP_HEIGHT 10
#define CUBE_SIZE 100

extern uint8_t world_map[MAP_HEIGHT][MAP_DEPTH][MAP_WIDTH];

void draw_map(uint8_t cam_angle_x, uint8_t cam_angle_y, uint8_t cam_angle_z, int cam_x, int cam_y, int cam_z);

#endif
