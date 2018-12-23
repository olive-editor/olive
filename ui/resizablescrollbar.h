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
    void resized_scroll(double d);
protected:
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
private:
    bool resize_init;
    bool resize_proc;
    int resize_start;
    bool resize_top;
};

#endif // RESIZABLESCROLLBAR_H
