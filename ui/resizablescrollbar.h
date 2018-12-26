#ifndef RESIZABLESCROLLBAR_H
#define RESIZABLESCROLLBAR_H

#include <QScrollBar>

class ResizableScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    ResizableScrollBar(QWidget * parent = 0);
    bool is_resizing();
signals:
    void resize_move(double i);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
private:
    bool resize_init;
    bool resize_proc;
    int resize_start;
    bool resize_top;

    int resize_start_max;
    int resize_start_width;
};

#endif // RESIZABLESCROLLBAR_H
