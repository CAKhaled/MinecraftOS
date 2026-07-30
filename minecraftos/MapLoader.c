#include "MapLoader.h"
#include "cuberenderer.h"


uint8_t world_map[MAP_HEIGHT][MAP_DEPTH][MAP_WIDTH];
static int map_initialized = 0;

void init_map_if_needed() {
    if (map_initialized) return;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int z = 0; z < MAP_DEPTH; z++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (y == 0) {
                    world_map[y][z][x] = 1; // Floor
                } else {
                    world_map[y][z][x] = 0; // Empty
                }
            }
        }
    }
    map_initialized = 1;
}

typedef struct {
    int x;
    int y;
    int z;
    long dist_sq;
    int is_hovered;
    uint8_t block_type;
} ActiveCube;

extern int hovered_grid_x;
extern int hovered_grid_y;
extern int hovered_grid_z;

void draw_map(uint8_t cam_angle_x, uint8_t cam_angle_y, uint8_t cam_angle_z, int cam_x, int cam_y, int cam_z) {
    init_map_if_needed();
    static ActiveCube cubes[8000]; // Increased capacity for 40x40 map
    int count = 0;

    // 1. Find all active cubes and calculate their squared distance to the camera
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int z = 0; z < MAP_DEPTH; z++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                uint8_t btype = world_map[y][z][x];
                if (btype != 0) {
                    if (count >= 8000) break;
                    
                    int world_x = (x - MAP_WIDTH / 2) * CUBE_SIZE;
                    int world_y = -y * CUBE_SIZE; // Y goes down (negative)
                    int world_z = z * CUBE_SIZE;
                    
                    long dx = (long)(world_x - cam_x);
                    long dy_dist = (long)(world_y - cam_y);
                    long dz = (long)(world_z - cam_z);
                    long dist = dx*dx + dy_dist*dy_dist + dz*dz;
                    
                    // Distance culling - increased for bigger map
                    if (dist > 3000000) continue;
                    
                    cubes[count].x = world_x;
                    cubes[count].y = world_y;
                    cubes[count].z = world_z;
                    cubes[count].dist_sq = dist;
                    cubes[count].is_hovered = (x == hovered_grid_x && y == hovered_grid_y && z == hovered_grid_z) ? 1 : 0;
                    cubes[count].block_type = btype;
                    count++;
                }
            }
        }
    }

    // 2. Sort cubes from furthest to nearest (Painter's Algorithm)
    // Insertion sort is better than bubble sort for larger arrays
    for (int i = 1; i < count; i++) {
        ActiveCube key = cubes[i];
        int j = i - 1;
        while (j >= 0 && cubes[j].dist_sq < key.dist_sq) {
            cubes[j + 1] = cubes[j];
            j = j - 1;
        }
        cubes[j + 1] = key;
    }

    // 3. Draw sorted cubes
    for (int i = 0; i < count; i++) {
        render_cube_at(cubes[i].x, cubes[i].y, cubes[i].z, 
                       cam_angle_x, cam_angle_y, cam_angle_z, 
                       cam_x, cam_y, cam_z, cubes[i].is_hovered,
                       cubes[i].block_type);
    }
}
