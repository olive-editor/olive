#ifndef LABELSLIDER_H
#define LABELSLIDER_H

#include <QLabel>
#include <QUndoCommand>

enum LabelSliderDisplayType {
    LABELSLIDER_NORMAL,
    LABELSLIDER_FRAMENUMBER,
    LABELSLIDER_PERCENT,
    LABELSLIDER_DECIBEL
};

class LabelSlider : public QLabel
{
    Q_OBJECT
public:
    LabelSlider(QWidget* parent = 0);
	void set_frame_rate(double d);
	void set_display_type(int type);
	void set_value(double v, bool userSet);
    void set_default_value(double v);
    void set_minimum_value(double v);
    void set_maximum_value(double v);
    double value();
    bool is_set();
	bool is_dragging();
	QString valueToString(double v);
	double getPreviousValue();
    void set_previous_value();
	void set_color(QString c = 0);
	int decimal_places;
protected:
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
private:
    double default_value;
    double internal_value;
    double drag_start_value;
	double previous_value;

    bool min_enabled;
    double min_value;
    bool max_enabled;
    double max_value;

    bool drag_start;
    bool drag_proc;
    int drag_start_x;
    int drag_start_y;

    bool set;

	int display_type;

	double frame_rate;

    void set_default_cursor();
    void set_active_cursor();
signals:
	void valueChanged();
	void clicked();
};

#endif // LABELSLIDER_H
