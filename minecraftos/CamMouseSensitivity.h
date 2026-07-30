#ifndef CAM_MOUSE_SENSITIVITY_H
#define CAM_MOUSE_SENSITIVITY_H

#include <stdint.h>

// Initializes the PS/2 mouse
void init_mouse();

// Polls the PS/2 mouse for data.
// Returns 1 if mouse data was read, 0 otherwise.
// dx and dy are populated with the movement deltas.
// buttons is populated with the button state (bit 0 = left click).
int poll_mouse(int* dx, int* dy, uint8_t* buttons);

#endif
