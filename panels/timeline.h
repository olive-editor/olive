#ifndef TIMELINE_H
#define TIMELINE_H

#include "ui/timelinetools.h"
#include "project/selection.h"

#include <QDockWidget>
#include <QVector>
#include <QTime>

#define TRACK_DEFAULT_HEIGHT 40

#define ADD_OBJ_TITLE 0
#define ADD_OBJ_SOLID 1
#define ADD_OBJ_BARS 2
#define ADD_OBJ_TONE 3
#define ADD_OBJ_NOISE 4
#define ADD_OBJ_AUDIO 5

class QPushButton;
class SourceTable;
class ViewerWidget;
class ComboAction;
class Effect;
class Transition;
struct EffectMeta;
struct Sequence;
struct Clip;
struct Media;
struct MediaStream;

long refactor_frame_number(long framenumber, double source_frame_rate, double target_frame_rate);
int getScreenPointFromFrame(double zoom, long frame);
long getFrameFromScreenPoint(double zoom, int x);
void draw_selection_rectangle(QPainter& painter, const QRect& rect);
bool selection_contains_transition(const Selection& s, Clip* c, int type);
void move_clip(ComboAction *ca, Clip *c, long iin, long iout, long iclip_in, int itrack, bool verify_transitions = true);

struct Ghost {
    int clip;
	long in;
	long out;
	int track;
	long clip_in;

	long old_in;
	long old_out;
	int old_track;
	long old_clip_in;

	// importing variables
    void* media;
    int media_type;
    int media_stream;

	// other variables
	long ghost_length;
	long media_length;
    bool trim_in;
    bool trimming;

	// transition trimming
    Transition* transition;
};

namespace Ui {
class Timeline;
}

class Timeline : public QDockWidget
{
	Q_OBJECT
public:
	explicit Timeline(QWidget *parent = 0);
	~Timeline();

    bool focused();
    void set_zoom(bool in);
    void copy(bool del);
    void paste(bool insert);
    void deselect();
    Clip* split_clip(ComboAction* ca, int p, long frame);
    Clip* split_clip(ComboAction* ca, int p, long frame, long post_in);
    bool split_selection(ComboAction* ca);
    void split_at_playhead();
    bool split_all_clips_at_point(ComboAction *ca, long point);
    bool split_clip_and_relink(ComboAction* ca, int clip, long frame, bool relink);
    void clean_up_selections(QVector<Selection>& areas);
	void deselect_area(long in, long out, int track);
    void delete_areas_and_relink(ComboAction *ca, QVector<Selection>& areas);
    void relink_clips_using_ids(QVector<int>& old_clips, QVector<Clip*>& new_clips);
    void update_sequence();
    void increase_track_height();
    void decrease_track_height();
    void add_transition();
    QVector<int> get_tracks_of_linked_clips(int i);
    bool has_clip_been_split(int c);
    void toggle_links();
	void ripple_to_in_point(bool in, bool ripple);
    void delete_in_out(bool ripple);
	void previous_cut();
	void next_cut();
	void toggle_show_all();

	void create_ghosts_from_media(Sequence *seq, long entry_point, QVector<void *> &media_list, QVector<int> &type_list);
	void add_clips_from_ghosts(ComboAction *ca, Sequence *s);

	int getTimelineScreenPointFromFrame(long frame);
	long getTimelineFrameFromScreenPoint(int x);
	int getDisplayScreenPointFromFrame(long frame);
	long getDisplayFrameFromScreenPoint(int x);

	int get_snap_range();
    bool snap_to_point(long point, long* l);
	bool snap_to_timeline(long* l, bool use_playhead, bool use_markers, bool use_workarea);
	void set_marker();

    // shared information
	int tool;
    long cursor_frame;
    int cursor_track;
    double zoom;
	long drag_frame_start;
	int drag_track_start;
	void update_effect_controls();
	bool showing_all;
	double old_zoom;

    QVector<int> video_track_heights;
    QVector<int> audio_track_heights;
    int get_track_height_size(bool video);
    int calculate_track_height(int track, int height);

    // snapping
    bool snapping;
    bool snapped;
    long snap_point;

	// selecting functions
	bool selecting;
    int selection_offset;
    bool is_clip_selected(Clip* clip, bool containing);
	void delete_selection(QVector<Selection> &selections, bool ripple);
	void select_all();
    bool rect_select_init;
    bool rect_select_proc;
    int rect_select_x;
    int rect_select_y;
    int rect_select_w;
    int rect_select_h;

	// moving
	bool moving_init;
	bool moving_proc;
    QVector<Ghost> ghosts;
    bool video_ghosts;
    bool audio_ghosts;
	bool move_insert;

	// trimming
	int trim_target;
    bool trim_in_point;
	int transition_select;

	// splitting
	bool splitting;
    QVector<int> split_tracks;
    QVector<int> split_cache;

	// importing
    bool importing;
	bool importing_files;

	// creating variables
	bool creating;
	int creating_object;

    // transition variables
    bool transition_tool_init;
    bool transition_tool_proc;
    int transition_tool_pre_clip;
    int transition_tool_post_clip;
    int transition_tool_type;
	const EffectMeta* transition_tool_meta;
	int transition_tool_side;

	// clipboard
	QVector<Clip*> clip_clipboard;

    Ui::Timeline *ui;
public slots:
	void repaint_timeline();

private slots:

	void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_snappingButton_toggled(bool checked);

    void on_toolSlideButton_clicked();

    void on_toolArrowButton_clicked();

    void on_toolEditButton_clicked();

    void on_toolRippleButton_clicked();

    void on_toolRollingButton_clicked();

    void on_toolRazorButton_clicked();

    void on_toolSlipButton_clicked();

	void on_addButton_clicked();

	void addMenuItem(QAction*);

	void setScroll(int);

	void on_recordButton_clicked();

	void on_toolTransitionButton_clicked();

	void transition_menu_select(QAction*);

private:
	void set_zoom_value(double v);
	QVector<QPushButton*> tool_buttons;
	void decheck_tool_buttons(QObject* sender);
	void set_tool(int tool);
    bool center_scroll_to_playhead();
	long last_frame;
	int scroll;

	int default_track_height;
};

#endif // TIMELINE_H
