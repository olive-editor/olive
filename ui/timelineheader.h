#ifndef TIMELINEHEADER_H
#define TIMELINEHEADER_H

#include <QWidget>

class TimelineHeader : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineHeader(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    bool dragging;

signals:

public slots:
};

#endif // TIMELINEHEADER_H
