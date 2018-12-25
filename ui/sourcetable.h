#ifndef SOURCETABLE_H
#define SOURCETABLE_H

#include <QTreeView>
#include <QTimer>
#include <QUndoCommand>

class Project;
class Media;

class SourceTable : public QTreeView
{
    Q_OBJECT
public:
    SourceTable(QWidget* parent = 0);
    Project* project_parent;
protected:
    void mousePressEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent *);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
private slots:
    void item_click(const QModelIndex& index);
    void show_context_menu();
};

#endif // SOURCETABLE_H
