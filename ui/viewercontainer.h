#ifndef VIEWERCONTAINER_H
#define VIEWERCONTAINER_H

#include <QWidget>
class ViewerWidget;

class ViewerContainer : public QWidget
{
	Q_OBJECT
public:
    explicit ViewerContainer(QWidget *parent = 0);
	float aspect_ratio;
    ViewerWidget* child;
	void adjust();

protected:
	void resizeEvent(QResizeEvent *event);

signals:

public slots:
};

#endif // VIEWERCONTAINER_H
