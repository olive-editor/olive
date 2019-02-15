#ifndef FOCUSFILTER_H
#define FOCUSFILTER_H

#include <QObject>

class FocusFilter : public QObject {
    Q_OBJECT
public:
    FocusFilter();

public slots:
    void cut();
    void copy();

    void duplicate();

    void go_to_in();
    void go_to_out();
    void go_to_start();
    void prev_frame();
    void play_in_to_out();
    void playpause();
    void pause();
    void increase_speed();
    void decrease_speed();
    void next_frame();
    void go_to_end();

    void set_viewer_fullscreen();

    void set_marker();

    void set_in_point();
    void set_out_point();
    void clear_in();
    void clear_out();
    void clear_inout();

    void delete_function();
    void select_all();

    void zoom_in();
    void zoom_out();
};

namespace Olive {
    extern FocusFilter FocusFilter;
}

#endif // FOCUSFILTER_H
