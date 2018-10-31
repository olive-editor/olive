#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

#define SAVE_VERSION 181030 // YYMMDD
#define MIN_SAVE_VERSION 180820 // lowest compatible project version

#define TIMECODE_DROP 0
#define TIMECODE_NONDROP 1
#define TIMECODE_FRAMES 2

#define RECORD_MODE_MONO 1
#define RECORD_MODE_STEREO 2

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
    int timecode_view;
    bool show_title_safe_area;
    bool use_custom_title_safe_ratio;
    double custom_title_safe_ratio;
	bool enable_drag_files_to_timeline;
    bool autoscale_by_default;
	int recording_mode;

    void load(QString path);
    void save(QString path);
};

extern Config config;

#endif // CONFIG_H
