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

    Viewer* viewer;
    ViewerWidget* child;
	void adjust();

protected:
	void resizeEvent(QResizeEvent *event);

signals:

public slots:

private:
    QWidget* area;
};

#endif // VIEWERCONTAINER_H
