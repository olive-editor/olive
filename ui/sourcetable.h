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
    void mouseDoubleClickEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
private:
    QTimer rename_timer;
    Media* editing_item;
    QModelIndex editing_index;
private slots:
    void rename_interval();
    void item_click(const QModelIndex& index);
    void stop_rename_timer();
    void item_renamed(Media *item);
	void show_context_menu();
	void create_seq_from_selected();
	void reveal_in_browser();
};

#endif // SOURCETABLE_H
