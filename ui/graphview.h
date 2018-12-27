#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QWidget>

class GraphView : public QWidget {
public:
    GraphView(QWidget* parent = 0);

    void paintEvent(QPaintEvent *event);
};

#endif // GRAPHVIEW_H
