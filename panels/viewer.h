#ifndef VIEWER_H
#define VIEWER_H

#include <QDockWidget>
#include <QTimer>
#include <QIcon>

class Timeline;
class ViewerWidget;
class Media;
struct Sequence;
class TimelineHeader;
class ResizableScrollBar;
class ViewerContainer;
class LabelSlider;
class QPushButton;
class QLabel;

bool frame_rate_is_droppable(float rate);
long timecode_to_frame(const QString& s, int view, double frame_rate);
QString frame_to_timecode(long f, int view, double frame_rate);

class Viewer : public QDockWidget
{
	Q_OBJECT

public:
	explicit Viewer(QWidget *parent = 0);
	~Viewer();

	bool is_focused();
	void set_main_sequence();
	void set_media(Media *m);
	void compose();
	void set_playpause_icon(bool play);
	void update_playhead_timecode(long p);
	void update_end_timecode();
	void update_header_zoom();
	void update_viewer();
    void clear_in();
    void clear_out();
	void clear_inout_point();
    void toggle_enable_inout();
	void set_in_point();
	void set_out_point();
	void set_zoom(bool in);

	// playback functions
	void seek(long p);
	void play();
	void pause();
	bool playing;
	long playhead_start;
	qint64 start_msecs;
	QTimer playback_updater;
	bool just_played;

	void cue_recording(long start, long end, int track);
	void uncue_recording();
	bool is_recording_cued();
	long recording_start;
	long recording_end;
	int recording_track;

	void reset_all_audio();
	void update_parents();

	ViewerWidget* viewer_widget;

	Media* media;
	Sequence* seq;

	void resizeEvent(QResizeEvent *event);

public slots:
	void play_wake();
	void go_to_start();
	void go_to_in();
	void previous_frame();
	void toggle_play();
	void next_frame();
	void go_to_out();
	void go_to_end();

private slots:
	void update_playhead();
	void timer_update();
	void recording_flasher_update();
	void resize_move(double d);

private:
	void clean_created_seq();
	void set_sequence(bool main, Sequence* s);
	bool main_sequence;
	bool created_sequence;
	long cached_end_frame;
	QString panel_name;
	double minimum_zoom;
	void set_zoom_value(double d);
	void set_sb_max();

	long get_seq_in();
	long get_seq_out();

	QIcon playIcon;

	void setup_ui();

	TimelineHeader* headers;
	ResizableScrollBar* horizontal_bar;
	ViewerContainer* viewer_container;
	LabelSlider* currentTimecode;
	QLabel* endTimecode;

	QPushButton* btnSkipToStart;
	QPushButton* btnRewind;
	QPushButton* btnPlay;
	QPushButton* btnFastForward;
	QPushButton* btnSkipToEnd;

	bool cue_recording_internal;
	QTimer recording_flasher;

	long previous_playhead;
};

#endif // VIEWER_H
