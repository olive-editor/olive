#ifndef SOURCEICONVIEW_H
#define SOURCEICONVIEW_H

#include <QListView>

class Project;

class SourceIconView : public QListView {
    Q_OBJECT
public:
    SourceIconView(QWidget* parent = 0);
    Project* project_parent;

    void mousePressEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent* event);
signals:
    void changed_root();
private slots:
    void show_context_menu();
    void item_click(const QModelIndex& index);
};

#endif // SOURCEICONVIEW_H
