#ifndef CANVAS_UI_H
#define CANVAS_UI_H

#include <stdint.h>

/* تهيئة نظام الـ UI (استدعِها مرة واحدة عند البداية) */
void canvas_ui_init(void);

/* ارسم الـ UI فوق الـ backbuffer - استدعِها بعد draw_map وقبل end_render */
void canvas_ui_draw(void);

/* تغيير الـ slot المحدد (0-8) */
void canvas_ui_set_slot(int slot);
int canvas_ui_get_selected(void);

#endif
