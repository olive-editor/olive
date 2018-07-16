#ifndef CONFIG_H
#define CONFIG_H

extern const char* version;
extern float scale;
extern bool vsync;
extern bool hwaccel;
//extern int padding;
extern bool custom_scale;
extern bool saved_layout;
extern bool show_track_lines;

extern bool scroll_zooms;

void load_config();
void save_config();

#endif // CONFIG_H
