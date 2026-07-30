#include <stdint.h>
void kernel_main(uint32_t magic, uint32_t addr) {
    uint32_t* mbd = (uint32_t*)addr;
    uint32_t fb_addr_low = mbd[22];
    uint32_t fb_width = mbd[25];
    uint32_t fb_height = mbd[26];
    uint8_t fb_bpp = ((uint8_t*)mbd)[108];
    uint8_t fb_type = ((uint8_t*)mbd)[109];
    
    // Draw something if valid
    if (fb_addr_low != 0 && fb_bpp == 32) {
        uint32_t* fb = (uint32_t*)fb_addr_low;
        for (uint32_t i = 0; i < fb_width * fb_height; i++) {
            fb[i] = 0x00FF0000; // RED
        }
    } else {
        // fallback to text mode to show error
        uint16_t* vga = (uint16_t*)0xB8000;
        vga[0] = (0x0F << 8) | 'E';
        vga[1] = (0x0F << 8) | 'R';
        vga[2] = (0x0F << 8) | 'R';
    }
    while(1);
}
