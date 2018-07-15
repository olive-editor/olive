#include "sourcetable.h"
#include "panels/project.h"

#include "io/media.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/panels.h"
#include "playback/playback.h"
#include "project/undo.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>

SourceTable::SourceTable(QWidget* parent) : QTreeWidget(parent) {
    sortByColumn(0, Qt::AscendingOrder);
    rename_timer.setInterval(500);
    connect(&rename_timer, SIGNAL(timeout()), this, SLOT(rename_interval()));
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(item_click(QTreeWidgetItem*,int)));
}

void SourceTable::stop_rename_timer() {
    rename_timer.stop();
}

void SourceTable::rename_interval() {
    stop_rename_timer();
    if (hasFocus() && editing_item != NULL) {
        editItem(editing_item, 0);
    }
}
void SourceTable::item_click(QTreeWidgetItem* item, int column) {
    if (column == 0) {
        editing_item = item;
        rename_timer.start();
    }
}

void SourceTable::mousePressEvent(QMouseEvent* event) {
    stop_rename_timer();
    QTreeWidget::mousePressEvent(event);
}

void SourceTable::mouseDoubleClickEvent(QMouseEvent* ) {
    stop_rename_timer();
	if (selectedItems().count() == 0) {
		panel_project->import_dialog();
	} else if (selectedItems().count() == 1) {
        QTreeWidgetItem* item = selectedItems().at(0);
        if (panel_project->get_type_from_tree(item) == MEDIA_TYPE_SEQUENCE) {
            set_sequence(panel_project->get_sequence_from_tree(item));
        }
    }
}

void SourceTable::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeWidget::dragEnterEvent(event);
    }
}

void SourceTable::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeWidget::dragMoveEvent(event);
    }
}

void SourceTable::dropEvent(QDropEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        // drag files in from outside
        QList<QUrl> urls = mimeData->urls();
        if (!urls.isEmpty()) {
            QStringList paths;
            for (int i=0;i<urls.size();i++) {
                qDebug() << urls.at(i).toLocalFile();
                paths.append(urls.at(i).toLocalFile());
            }
            panel_project->process_file_list(paths);
        }
        event->acceptProposedAction();
    } else {
        // dragging files within project
        QVector<QTreeWidgetItem*> move_items;
        QTreeWidgetItem* drop_item = itemAt(event->pos());
        // if we dragged to the root OR dragged to a folder
        if (drop_item == NULL || (drop_item != NULL && panel_project->get_type_from_tree(drop_item) == MEDIA_TYPE_FOLDER)) {
            QList<QTreeWidgetItem*> selected_items = selectedItems();
            for (int i=0;i<selected_items.size();i++) {
                QTreeWidgetItem* s = selected_items.at(i);
                bool ignore = false;
                if (s->parent() != NULL) {
                    // if child belongs to a selected parent, assume the user is just moving the parent and ignore the child
                    QTreeWidgetItem* par = s->parent();
                    while (par != NULL) {
                        for (int j=0;j<selected_items.size();j++) {
                            if (par == selected_items.at(j)) {
                                ignore = true;
                                break;
                            }
                        }
                        par = par->parent();
                    }
                }
                if (!ignore) {
                    move_items.append(s);
                }
            }
            if (move_items.size() > 0) {
                MediaMove* mm = new MediaMove(this);
                mm->to = drop_item;
                mm->items = move_items;
                undo_stack.push(mm);
                project_changed = true;
            }
        }
    }
}

MediaMove::MediaMove(SourceTable *s) : table(s) {}

void MediaMove::undo() {
    for (int i=0;i<items.size();i++) {
        if (to == NULL) {
            table->takeTopLevelItem(table->indexOfTopLevelItem(items.at(i)));
        } else {
            to->removeChild(items.at(i));
        }
        if (froms.at(i) == NULL) {
            table->addTopLevelItem(items.at(i));
        } else {
            froms.at(i)->addChild(items.at(i));
        }
    }
}

void MediaMove::redo() {
    for (int i=0;i<items.size();i++) {
        QTreeWidgetItem* parent = items.at(i)->parent();
        froms.append(parent);
        if (parent == NULL) {
            table->takeTopLevelItem(table->indexOfTopLevelItem(items.at(i)));
        } else {
            parent->removeChild(items.at(i));
        }
        if (to == NULL) {
            table->addTopLevelItem(items.at(i));
        } else {
            to->addChild(items.at(i));
        }
    }
}
