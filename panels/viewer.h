#ifndef VIEWER_H
#define VIEWER_H

#include <QDockWidget>

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
	void update_media(int type, void* media);
	void compose();
    void set_playpause_icon(bool play);
    void update_playhead_timecode(long p);
    void update_end_timecode();

	ViewerWidget* viewer_widget;

    Ui::Viewer *ui;
private slots:
	void on_pushButton_clicked();

	void on_pushButton_5_clicked();

	void on_pushButton_2_clicked();

	void on_pushButton_4_clicked();

    void on_pushButton_3_clicked();

	void update_playhead();
};

#endif // VIEWER_H
