#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *g_top_bottom_stone_path = NULL;
const char *g_sides_stone_path = NULL;

void load_config(void) {
    FILE *f = fopen("config.txt", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0; // trim newline
        if (strncmp(line, "top_bottom_stone=", 17) == 0) {
            g_top_bottom_stone_path = strdup(line + 17);
        } else if (strncmp(line, "sides_stone=", 12) == 0) {
            g_sides_stone_path = strdup(line + 12);
        }
    }
    fclose(f);
}
