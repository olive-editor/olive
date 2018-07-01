#ifndef SOURCETABLE_H
#define SOURCETABLE_H

#include <QTreeWidget>

class Project;

class SourceTable : public QTreeWidget
{
public:
	SourceTable(QWidget* parent = 0);
protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
private:
};

#endif // SOURCETABLE_H
