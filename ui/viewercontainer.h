#ifndef VIEWERCONTAINER_H
#define VIEWERCONTAINER_H

#include <QWidget>

class ViewerContainer : public QWidget
{
	Q_OBJECT
public:
    explicit ViewerContainer(QWidget *parent = 0);
	float aspect_ratio;
	QWidget* child;
	void adjust();

protected:
    void resizeEvent(QResizeEvent *event);

signals:

public slots:
};

#endif // VIEWERCONTAINER_H
