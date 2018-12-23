#ifndef VIEWERCONTAINER_H
#define VIEWERCONTAINER_H

#include <QScrollArea>
class Viewer;
class ViewerWidget;

class ViewerContainer : public QScrollArea
{
	Q_OBJECT
public:
    explicit ViewerContainer(QWidget *parent = 0);
    ~ViewerContainer();

    bool fit;
    double zoom;

    void dragScrollPress(const QPoint&);
    void dragScrollMove(const QPoint&);

    Viewer* viewer;
    ViewerWidget* child;
	void adjust();

protected:
	void resizeEvent(QResizeEvent *event);

signals:

public slots:

private:
    QWidget* area;
    int drag_start_x;
    int drag_start_y;
    int horiz_start;
    int vert_start;
};

#endif // VIEWERCONTAINER_H
