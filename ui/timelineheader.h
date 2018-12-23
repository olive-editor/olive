#ifndef TIMELINEHEADER_H
#define TIMELINEHEADER_H

#include <QWidget>
#include <QFontMetrics>
class Viewer;
class QScrollBar;

bool center_scroll_to_playhead(QScrollBar* bar, double zoom, long playhead);

class TimelineHeader : public QWidget
{
    Q_OBJECT
public:
	explicit TimelineHeader(QWidget *parent = 0);
    void set_in_point(long p);
    void set_out_point(long p);

	Viewer* viewer;

	bool snapping;

	void show_text(bool enable);
	void update_zoom(double z);
    double get_zoom();
	void delete_markers();
    void set_scrollbar_max(QScrollBar* bar, long sequence_end_frame, int offset);

public slots:
	void set_scroll(int);
	void set_visible_in(long i);
    void show_context_menu(const QPoint &pos);
    void resized_scroll_listener(double d);

protected:
	void paintEvent(QPaintEvent*);
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void mouseReleaseEvent(QMouseEvent*);
	void focusOutEvent(QFocusEvent*);

private:
	void update_parents();

    bool dragging;

    bool resizing_workarea;
    bool resizing_workarea_in;
    long temp_workarea_in;
    long temp_workarea_out;
    long sequence_end;

	double zoom;

    long in_visible;

	void set_playhead(int mouse_x);

    QFontMetrics fm;

	int drag_start;
	bool dragging_markers;
	QVector<int> selected_markers;
	QVector<long> selected_marker_original_times;

	long getHeaderFrameFromScreenPoint(int x);
	int getHeaderScreenPointFromFrame(long frame);

	int scroll;

	int height_actual;
	bool text_enabled;

signals:
};

#endif // TIMELINEHEADER_H
