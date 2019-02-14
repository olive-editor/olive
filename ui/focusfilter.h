#ifndef FOCUSFILTER_H
#define FOCUSFILTER_H

#include <QObject>

class FocusFilter : public QObject {
    Q_OBJECT
public:
    FocusFilter();

public slots:
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
};

namespace Olive {
    extern FocusFilter FocusFilter;
}

#endif // FOCUSFILTER_H
