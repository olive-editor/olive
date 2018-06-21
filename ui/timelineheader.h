#ifndef TIMELINEHEADER_H
#define TIMELINEHEADER_H

#include <QWidget>

class TimelineHeader : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineHeader(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private:
    bool dragging;

signals:

public slots:
};

#endif // TIMELINEHEADER_H
