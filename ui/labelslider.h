#ifndef LABELSLIDER_H
#define LABELSLIDER_H

#include <QLabel>

class LabelSlider : public QLabel
{
    Q_OBJECT
public:
    LabelSlider(QWidget* parent = 0);
    void set_value(float v);
    void set_default_value(float v);
    void set_minimum_value(float v);
    void set_maximum_value(float v);
    float value();
protected:
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
private:
    float internal_value;
    float default_value;

    bool min_enabled;
    float min_value;
    bool max_enabled;
    float max_value;

    bool drag_start;
    bool drag_proc;
    int drag_start_x;
    int drag_start_y;
signals:
    void valueChanged();
};

#endif // LABELSLIDER_H
