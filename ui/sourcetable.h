#ifndef SOURCETABLE_H
#define SOURCETABLE_H

#include <QTreeWidget>

class Project;

class SourceTable : public QTreeWidget
{
public:
	SourceTable(QWidget* parent = 0);
protected:
	void mouseDoubleClickEvent(QMouseEvent *event) override;
//	void dragEnterEvent(QDragEnterEvent *event) override;
private:
};

#endif // SOURCETABLE_H
