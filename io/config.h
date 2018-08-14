#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

#define SAVE_VERSION "180814"

struct Config {
    Config();
    bool saved_layout;
    bool show_track_lines;
    bool scroll_zooms;
    bool edit_tool_selects_links;
    bool edit_tool_also_seeks;
    bool select_also_seeks;
    bool paste_seeks;
    QString img_seq_formats;
	bool rectified_waveforms;
	int default_transition_length;

    void load(QString path);
    void save(QString path);
};

extern Config config;

#endif // CONFIG_H
