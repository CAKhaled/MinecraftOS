#include "CameraMove.h"
#include "os_config.h"
#include "CamMouseSensitivity.h"
#include "math3d.h"
#include "BuilderBreaker.h"

#include "MapLoader.h"

static int floor_div(long a, int b) {
    int res = a / b;
    int rem = a % b;
    if (rem != 0 && ((a < 0) ^ (b < 0))) {
        res -= 1;
    }
    return res;
}

static int is_solid(int x, int y, int z) {
    int grid_x = floor_div(x + 50, CUBE_SIZE) + (MAP_WIDTH / 2);
    int grid_y = floor_div(-y + 50, CUBE_SIZE);
    int grid_z = floor_div(z + 50, CUBE_SIZE);
    
    if (grid_x >= 0 && grid_x < MAP_WIDTH &&
        grid_y >= 0 && grid_y < MAP_HEIGHT &&
        grid_z >= 0 && grid_z < MAP_DEPTH) {
        return world_map[grid_y][grid_z][grid_x];
    }
    return 0; // Out of bounds is empty
}

int cam_x = 0;
int cam_y = 0;
int cam_z = 0;

int internal_cam_x = 0;
int internal_cam_y = 0;
int internal_cam_z = 0;

int cam_angle_x = STATIC_ANGLE_X;
int cam_angle_y = STATIC_ANGLE_Y;

static uint8_t key_state[256] = {0};
static int y_velocity = 0;

void init_camera() {
    cam_x = 0;
    cam_y = STATIC_BLOCK_MAP_Y_AXIS;
    cam_z = 0;
    internal_cam_x = 0;
    internal_cam_y = STATIC_BLOCK_MAP_Y_AXIS * 256;
    internal_cam_z = 0;
    cam_angle_x = STATIC_ANGLE_X;
    cam_angle_y = STATIC_ANGLE_Y;
}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t last_kb_time = 0;

void update_camera() {
    // 1. Drain keyboard buffer and update key states
    uint8_t scancode;
    int got_input = 0;
    while ((scancode = read_scancode()) != 0) {
        got_input = 1;
        if (scancode == 0x0E || scancode == 0x01) { // Backspace or Esc
            for (int i = 0; i < 256; i++) {
                key_state[i] = 0;
            }
            continue;
        }
        if (scancode < 0x80) {
            key_state[scancode] = 1; // Key pressed
        } else {
            key_state[scancode - 0x80] = 0; // Key released
        }
    }

    uint64_t current = rdtsc();
    if (got_input) {
        last_kb_time = current;
    } else {
        int any_pressed = 0;
        for (int i = 0; i < 256; i++) {
            if (key_state[i]) {
                any_pressed = 1;
                break;
            }
        }
        if (any_pressed) {
            // If keys are active but no keyboard packets received for ~0.5 seconds,
            // assume QEMU lost focus or key release was missed, and reset all keys.
            // if (current - last_kb_time > 1500000000ULL) {
            //     for (int i = 0; i < 256; i++) {
            //         key_state[i] = 0;
            //     }
            //     last_kb_time = current;
            // }
        } else {
            last_kb_time = current;
        }
    }

    // 2. Process smooth directional movement every frame
    // Left Shift (0x2A) = Sprint (3x speed)
    int is_sprinting = key_state[0x2A]; // Left Shift

    // Calculate full 3D Forward and Right vectors (Unity Vector3.forward style)
    int pitch = (uint8_t)cam_angle_x;
    int yaw = (uint8_t)cam_angle_y;

    int cos_pitch = get_cos(pitch);
    int sin_pitch = get_sin(pitch);
    int cos_yaw = get_cos(yaw);
    int sin_yaw = get_sin(yaw);

    // Forward vector ignoring pitch to prevent flying (FPS Walk style)
    int fwd_x = -sin_yaw;
    int fwd_y = 0;
    int fwd_z = cos_yaw;
    
    // Vector3.right = (cosYaw, 0, sinYaw)
    int right_x = cos_yaw;
    int right_z = sin_yaw;

    // Base speed: multiply by 4 for normal, by 12 for sprint
    int speed_mul = is_sprinting ? 12 : 4;

    // Backup old positions for collision
    int old_x = internal_cam_x;
    int old_z = internal_cam_z;

    if (key_state[0x11]) {
        internal_cam_x += fwd_x * speed_mul;
        internal_cam_z += fwd_z * speed_mul;
    } 
    if (key_state[0x1F]) {
        internal_cam_x -= fwd_x * speed_mul;
        internal_cam_z -= fwd_z * speed_mul;
    } 
    if (key_state[0x20]) {
        internal_cam_x += right_x * speed_mul;
        internal_cam_z += right_z * speed_mul;
    }
    if (key_state[0x1E]) {
        internal_cam_x -= right_x * speed_mul;
        internal_cam_z -= right_z * speed_mul;
    }

    // X/Z Collision
    // We check feet and head. Feet = cam_y + 100, Head = cam_y
    int check_x = internal_cam_x / 256;
    int check_z = internal_cam_z / 256;
    int check_feet = (internal_cam_y / 256) + 100;
    int check_head = (internal_cam_y / 256);
    
    if (is_solid(check_x, check_feet, check_z) || is_solid(check_x, check_head, check_z)) {
        // Simple resolution: just revert movement entirely if we hit a wall
        // In a real engine, we would slide along walls.
        internal_cam_x = old_x;
        internal_cam_z = old_z;
    }

    // Jump logic and Gravity
    internal_cam_y -= y_velocity;
    y_velocity -= 80; // Gravity (scaled by 256)
    
    // Check floor collision dynamically
    int floor_y = STATIC_BLOCK_MAP_Y_AXIS * 256;
    int grid_x = floor_div((internal_cam_x/256) + 50, CUBE_SIZE) + (MAP_WIDTH / 2);
    int grid_z = floor_div((internal_cam_z/256) + 50, CUBE_SIZE);
    
    if (grid_x >= 0 && grid_x < MAP_WIDTH && grid_z >= 0 && grid_z < MAP_DEPTH) {
        int feet_y = (internal_cam_y / 256) + 110;
        for (int y = MAP_HEIGHT - 1; y >= 0; y--) {
            if (world_map[y][grid_z][grid_x] != 0) {
                int block_top_y = -y * CUBE_SIZE - 50;
                // Only snap to this block if we are falling onto it
                if (feet_y <= block_top_y + 50) {
                    floor_y = (block_top_y - 110) * 256;
                    break;
                }
            }
        }
    }

    if (internal_cam_y >= floor_y) {
        internal_cam_y = floor_y;
        y_velocity = 0;
        
        // Jump if Spacebar is pressed and we are on the ground
        if (key_state[0x39]) {
            y_velocity = STATIC_JUMP_HEIGHT * 20; 
        }
    }


    // Convert internal scaled coordinates back to integer space
    cam_x = internal_cam_x / 256;
    cam_y = internal_cam_y / 256;
    cam_z = internal_cam_z / 256;

    // 3. Process mouse (for rotation)
    int dx = 0, dy = 0;
    uint8_t buttons = 0;
    while (poll_mouse(&dx, &dy, &buttons)) {
        // We use a while loop to drain the mouse buffer if multiple events are pending
        // Apply sensitivity from config
        // العودة للنظام الطبيعي للألعاب (FPS):
        cam_angle_y -= dx * STATIC_CAM_SENSITIVITY;
        cam_angle_x -= dy * STATIC_CAM_SENSITIVITY; 
        
        // Clamp X angle (Pitch) between -64 and 64 (which is -90 to 90 degrees)
        if (cam_angle_x > 64) cam_angle_x = 64;
        if (cam_angle_x < -64) cam_angle_x = -64;
        
        // Wrap Y angle (Yaw) within 0-255 since we use a 256-element sine table
        if (cam_angle_y < 0) cam_angle_y += 256;
        if (cam_angle_y > 255) cam_angle_y %= 256;
    }
    
    // 4. Update BuilderBreaker with current state
    update_builder_breaker(buttons, cam_x, cam_y, cam_z, cam_angle_x, cam_angle_y);
}

/* يرجع 1 إذا الزر محروس، 0 إذا لا */
int get_key_state(uint8_t scancode) {
    return key_state[scancode] ? 1 : 0;
}
