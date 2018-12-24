#ifndef SOURCEICONVIEW_H
#define SOURCEICONVIEW_H

#include <QListView>

class Project;

class SourceIconView : public QListView {
    Q_OBJECT
public:
    SourceIconView(QWidget* parent = 0);
    Project* project_parent;
    void mouseDoubleClickEvent(QMouseEvent *event);
signals:
    void changed_root();
};

#endif // SOURCEICONVIEW_H
