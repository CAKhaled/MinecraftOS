#ifndef CAMERAMOVE_H
#define CAMERAMOVE_H

#include <stdint.h>

// Assembly function to read the keyboard
extern uint8_t read_scancode();

// Camera state variables
extern int cam_x;
extern int cam_y;
extern int cam_z;

// Camera rotation angles
extern int cam_angle_x;
extern int cam_angle_y;

// Initialize camera variables
void init_camera();

// Update camera position based on keyboard input and mouse
void update_camera();

// Query if a key is currently held (by PS/2 scan code)
int get_key_state(uint8_t scancode);

#endif
