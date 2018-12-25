#ifndef SOURCESCOMMON_H
#define SOURCESCOMMON_H

#include <QModelIndexList>
#include <QTimer>

class Project;
class QMouseEvent;
class Media;
class QAbstractItemView;
class QDropEvent;

class SourcesCommon : public QObject {
    Q_OBJECT
public:
    SourcesCommon(Project *parent);
    QAbstractItemView* view;
    void show_context_menu(QWidget* parent, const QModelIndexList &items);

    void mousePressEvent(QMouseEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e, const QModelIndexList& selected_items);
    void dropEvent(QWidget *parent, QDropEvent* e, const QModelIndex& drop_item, const QModelIndexList &items);

    void item_click(Media* m, const QModelIndex &index);
private slots:
    void create_seq_from_selected();
    void reveal_in_browser();
    void rename_interval();
    void item_renamed(Media *item);
private:
    Media* editing_item;
    QModelIndex editing_index;
    QModelIndexList selected_items;
    Project* project_parent;
    void stop_rename_timer();
    QTimer rename_timer;
};

#endif // SOURCESCOMMON_H
