#ifndef VIEWERCONTAINER_H
#define VIEWERCONTAINER_H

#include <QWidget>
class Viewer;
class ViewerWidget;
class QScrollBar;

class ViewerContainer : public QWidget
{
	Q_OBJECT
public:
	explicit ViewerContainer(QWidget *parent = 0);
	~ViewerContainer();

	bool fit;
	double zoom;

	void dragScrollPress(const QPoint&);
	void dragScrollMove(const QPoint&);
	void parseWheelEvent(QWheelEvent* event);

	Viewer* viewer;
	ViewerWidget* child;
	void adjust();

protected:
	void resizeEvent(QResizeEvent *event);

signals:

public slots:

private slots:
	void scroll_changed();

private:
	int drag_start_x;
	int drag_start_y;
	int horiz_start;
	int vert_start;
	QScrollBar* horizontal_scrollbar;
	QScrollBar* vertical_scrollbar;
};

#endif // VIEWERCONTAINER_H
