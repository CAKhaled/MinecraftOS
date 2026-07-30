#include "BuilderBreaker.h"
#include "MapLoader.h"
#include "math3d.h"

static int last_buttons = 0;
static int cooldown = 0;

int hovered_grid_x = -1;
int hovered_grid_y = -1;
int hovered_grid_z = -1;

static int floor_div(long a, int b) {
    int res = a / b;
    int rem = a % b;
    if (rem != 0 && ((a < 0) ^ (b < 0))) {
        res -= 1;
    }
    return res;
}

void update_builder_breaker(uint8_t buttons, int cam_x, int cam_y, int cam_z, int pitch, int yaw) {
    if (cooldown > 0) {
        cooldown--;
    }
    
    // Left click is bit 0, Right click is bit 1
    int is_left_click = (buttons & 0x01) && !(last_buttons & 0x01);
    int is_right_click = (buttons & 0x02) && !(last_buttons & 0x02);
    last_buttons = buttons;
    
    // We still run the raycast to update hovered_grid for rendering.
    
    // Calculate forward vector based on pitch and yaw
    // The forward vector in 3D:
    // fwd.x = -cos(pitch) * sin(yaw)
    // fwd.y = sin(pitch)
    // fwd.z = cos(pitch) * cos(yaw)
    
    int cos_pitch = get_cos(pitch);
    int sin_pitch = get_sin(pitch);
    // The math functions were redefined later, but we can just skip this
    // since we redefine them in the next lines.
    
    // Actually, math3d.h only exposes get_cos(angle) and get_sin(angle).
    // Let me fix that. It's just get_cos and get_sin.
    int cos_yaw = get_cos(yaw);
    int sin_yaw = get_sin(yaw);
    
    long fwd_x = -(cos_pitch * sin_yaw) / 256;
    long fwd_y = sin_pitch;
    long fwd_z = (cos_pitch * cos_yaw) / 256;
    
    // Raycast: Step forward from camera position
    long ray_x = cam_x;
    long ray_y = cam_y;
    long ray_z = cam_z;
    
    // We keep track of the PREVIOUS empty grid position, to place the block ON TOP of the hit block
    int prev_grid_x = -1, prev_grid_y = -1, prev_grid_z = -1;
    
    // Step size (small enough to not skip blocks, large enough for performance)
    // CUBE_SIZE is 100. Step size 10 is very safe.
    int step = 10;
    
    // Maximum reach distance: 4 blocks (400 units)
    for (int t = 0; t < 400; t += step) {
        ray_x += (fwd_x * step) / 256;
        ray_y += (fwd_y * step) / 256;
        ray_z += (fwd_z * step) / 256;
        
        // Convert world coordinates back to grid coordinates
        // We add 50 (half CUBE_SIZE) because the visual cubes are centered at 0, 100, 200, etc.
        // meaning their boundaries are at -50 to 50, 50 to 150, etc.
        int grid_x = floor_div(ray_x + 50, CUBE_SIZE) + (MAP_WIDTH / 2);
        
        // y is inverted in world space (y goes down negative)
        int grid_y = floor_div(-ray_y + 50, CUBE_SIZE);
        
        int grid_z = floor_div(ray_z + 50, CUBE_SIZE);
        
        // Check bounds
        if (grid_x >= 0 && grid_x < MAP_WIDTH &&
            grid_y >= 0 && grid_y < MAP_HEIGHT &&
            grid_z >= 0 && grid_z < MAP_DEPTH) {
            
            // Did we hit a solid block?
            if (world_map[grid_y][grid_z][grid_x] != 0) {
                // Update hovered block for rendering
                hovered_grid_x = grid_x;
                hovered_grid_y = grid_y;
                hovered_grid_z = grid_z;
                
                if (is_right_click && prev_grid_x != -1 && prev_grid_y != -1 && prev_grid_z != -1) {
                    if (world_map[prev_grid_y][prev_grid_z][prev_grid_x] == 0) {
                        extern int canvas_ui_get_selected(void);
                        int selected = canvas_ui_get_selected();
                        world_map[prev_grid_y][prev_grid_z][prev_grid_x] = (selected == 1) ? 2 : 1;
                        cooldown = 10; // Prevent spamming
                    }
                }
                
                if (is_left_click) {
                    world_map[grid_y][grid_z][grid_x] = 0; // Break block
                    cooldown = 10; // Prevent spamming
                }
                
                return; // Stop raycast after hitting the first block
            } else {
                // Update previous empty position
                prev_grid_x = grid_x;
                prev_grid_y = grid_y;
                prev_grid_z = grid_z;
            }
        }
    }
    
    // If raycast didn't hit anything, clear hovered block
    hovered_grid_x = -1;
    hovered_grid_y = -1;
    hovered_grid_z = -1;
}
