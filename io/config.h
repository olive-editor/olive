#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

#define SAVE_VERSION 190104 // YYMMDD
#define MIN_SAVE_VERSION 190104 // lowest compatible project version

#define TIMECODE_DROP 0
#define TIMECODE_NONDROP 1
#define TIMECODE_FRAMES 2
#define TIMECODE_MILLISECONDS 3

#define RECORD_MODE_MONO 1
#define RECORD_MODE_STEREO 2

#define AUTOSCROLL_NO_SCROLL 0
#define AUTOSCROLL_PAGE_SCROLL 1
#define AUTOSCROLL_SMOOTH_SCROLL 2

#define PROJECT_VIEW_TREE 0
#define PROJECT_VIEW_ICON 1

#define FRAME_QUEUE_TYPE_FRAMES 0
#define FRAME_QUEUE_TYPE_SECONDS 1

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
	bool enable_seek_to_import;
	bool enable_audio_scrubbing;
	bool drop_on_media_to_replace;
	int autoscroll;
	int audio_rate;
	bool fast_seeking;
	bool hover_focus;
	int project_view_type;
	bool set_name_with_marker;
	bool show_project_toolbar;
	bool disable_multithreading_for_images;
	double previous_queue_size;
	int previous_queue_type;
	double upcoming_queue_size;
	int upcoming_queue_type;
    bool loop;
    bool pause_at_out_point;

	void load(QString path);
	void save(QString path);
};

extern Config config;

#endif // CONFIG_H
