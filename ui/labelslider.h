#ifndef LABELSLIDER_H
#define LABELSLIDER_H

#include <QLabel>
#include <QUndoCommand>

class LabelSlider : public QLabel
{
    Q_OBJECT
public:
    LabelSlider(QWidget* parent = 0);
    void set_value(double v);
    void set_default_value(double v);
    void set_minimum_value(double v);
    void set_maximum_value(double v);
    double value();
    bool is_set();
protected:
    void mousePressEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
private:
    double default_value;
    double internal_value;
    double drag_start_value;

    bool min_enabled;
    double min_value;
    bool max_enabled;
    double max_value;

    bool drag_start;
    bool drag_proc;
    int drag_start_x;
    int drag_start_y;

    bool set;
signals:
    void valueChanged();
};

class ValueChangeCommand : public QUndoCommand {
public:
    ValueChangeCommand();
    LabelSlider* source;
    float old_val;
    float new_val;
    void undo();
    void redo();
private:
    bool done;
};

#endif // LABELSLIDER_H
