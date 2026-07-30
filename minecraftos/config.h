#ifndef CONFIG_H
#define CONFIG_H

// Global texture path strings (read from config.txt)
extern const char *g_top_bottom_stone_path;
extern const char *g_sides_stone_path;

// Load config values at startup
void load_config(void);

#endif // CONFIG_H
