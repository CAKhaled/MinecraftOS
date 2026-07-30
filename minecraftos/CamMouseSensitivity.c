#include "CamMouseSensitivity.h"

extern uint8_t mouse_inb(uint16_t port);
extern void mouse_outb(uint16_t port, uint8_t val);

// Wait for mouse to be ready for writing
static inline void mouse_wait_write() {
    int timeout = 100000;
    while (timeout--) {
        if ((mouse_inb(0x64) & 2) == 0) {
            return;
        }
    }
}

// Wait for mouse data to be ready for reading
static inline void mouse_wait_read() {
    int timeout = 100000;
    while (timeout--) {
        if ((mouse_inb(0x64) & 1) == 1) {
            return;
        }
    }
}

static inline void mouse_write(uint8_t a_write) {
    mouse_wait_write();
    mouse_outb(0x64, 0xD4); // Tell keyboard controller to send command to mouse
    mouse_wait_write();
    mouse_outb(0x60, a_write); // Send the actual command
}

static inline uint8_t mouse_read() {
    mouse_wait_read();
    return mouse_inb(0x60);
}

void init_mouse() {
    uint8_t status;
    
    // Enable the auxiliary mouse device
    mouse_wait_write();
    mouse_outb(0x64, 0xA8);
    
    // Read COMPAQ status byte
    mouse_wait_write();
    mouse_outb(0x64, 0x20);
    mouse_wait_read();
    status = mouse_inb(0x60);
    
    // Modify status byte: disable mouse clock (bit 5)
    status &= ~0x20; // Clear bit 5
    
    // Write COMPAQ status byte
    mouse_wait_write();
    mouse_outb(0x64, 0x60);
    mouse_wait_write();
    mouse_outb(0x60, status);
    
    // Set default settings
    mouse_write(0xF6);
    mouse_read(); // Acknowledge
    
    // Enable data reporting
    mouse_write(0xF4);
    mouse_read(); // Acknowledge
}

int poll_mouse(int* dx, int* dy, uint8_t* buttons) {
    while (1) {
        uint8_t status = mouse_inb(0x64);
        // Check if data is available (bit 0) AND data is from mouse (bit 5)
        if ((status & 0x01) && (status & 0x20)) {
            // Read byte 1
            uint8_t b1 = mouse_inb(0x60);
            
            // Validation 1: Bit 3 of the first byte must be 1 in standard PS/2 protocol
            if ((b1 & 0x08) == 0) continue; // Instantly drop and try next byte
            
            // Validation 2: Ignore if overflow bits are set (prevents crazy jumps)
            if (b1 & 0xC0) continue; // Instantly drop

            // Wait and read byte 2
            int timeout = 100000;
            int aborted = 0;
            while (timeout--) {
                status = mouse_inb(0x64);
                if (status & 1) {
                    if (!(status & 0x20)) { aborted = 1; break; } // Keyboard data! Abort!
                    break;
                }
            }
            if (aborted || timeout <= 0) return 0;
            uint8_t b2 = mouse_inb(0x60);
            
            // Wait and read byte 3
            timeout = 100000;
            while (timeout--) {
                status = mouse_inb(0x64);
                if (status & 1) {
                    if (!(status & 0x20)) { aborted = 1; break; } // Keyboard data! Abort!
                    break;
                }
            }
            if (aborted || timeout <= 0) return 0;
            uint8_t b3 = mouse_inb(0x60);
            
            // Parse the packet
            int x = (int8_t)b2;
            int y = (int8_t)b3;
            
            // Handle negative signs correctly using bit 4 and 5 of b1
            if (b1 & 0x10) x |= 0xFFFFFF00;
            if (b1 & 0x20) y |= 0xFFFFFF00;
            
            if (x > 100 || x < -100 || y > 100 || y < -100) continue; // Glitch, ignore
            
            *dx = x;
            *dy = y; // Invert Y (was -y) to match user preference
            
            if (buttons) {
                *buttons = b1 & 0x07;
            }
            
            return 1;
        } else {
            // Buffer empty, or contains keyboard data
            return 0;
        }
    }
}
