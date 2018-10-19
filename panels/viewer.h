#ifndef VIEWER_H
#define VIEWER_H

#include <QDockWidget>
#include <QTimer>

class Timeline;
class ViewerWidget;
struct Sequence;

namespace Ui {
class Viewer;
}

bool frame_rate_is_droppable(float rate);
QString frame_to_timecode(long f, int view, double frame_rate);

class Viewer : public QDockWidget
{
	Q_OBJECT

public:
	explicit Viewer(QWidget *parent = 0);
	~Viewer();

	bool is_focused();
    void set_main_sequence();
    void set_media(int type, void* media);
	void compose();
    void set_playpause_icon(bool play);
    void update_playhead_timecode(long p);
    void update_end_timecode();
	void update_header_zoom();
	void update_viewer();
    void clear_inout_point();
	void set_in_point();
	void set_out_point();

	// playback functions
	void go_to_start();
	void previous_frame();
	void next_frame();
	void seek(long p);
	void toggle_play();
	void play();
	void pause();
	void go_to_end();
	bool playing;
	long playhead_start;
	qint64 start_msecs;
	QTimer playback_updater;

	void reset_all_audio();
	void update_parents();

	ViewerWidget* viewer_widget;

	Sequence* seq;

    Ui::Viewer *ui;

public slots:

private slots:
	void on_pushButton_clicked();
    void on_pushButton_5_clicked();
	void on_pushButton_2_clicked();
	void on_pushButton_4_clicked();
    void on_pushButton_3_clicked();
	void update_playhead();
	void timer_update();
private:
	void clean_created_seq();
    void set_sequence(bool main, Sequence* s);
	bool main_sequence;
	bool created_sequence;
};

#endif // VIEWER_H
