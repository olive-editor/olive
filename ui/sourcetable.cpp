#include "sourcetable.h"
#include "panels/project.h"

#include "io/media.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/panels.h"
#include "playback/playback.h"

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
        QTreeWidgetItem* item = itemAt(event->pos());
        if (item == NULL || (item != NULL && panel_project->get_type_from_tree(item) == MEDIA_TYPE_FOLDER)) {
            QList<QTreeWidgetItem*> items = selectedItems();
            for (int i=0;i<items.size();i++) {
                QTreeWidgetItem* s = items.at(i);
                bool ignore = false;
                if (s->parent() == NULL) {
                    takeTopLevelItem(indexOfTopLevelItem(s));
                } else {
                    // if child belongs to a selected parent, assume the user is just moving the parent and ignore the child
                    for (int j=0;j<items.size();j++) {
                        if (s->parent() == items.at(j)) {
                            ignore = true;
                            break;
                        }
                    }
                    if (!ignore) s->parent()->takeChild(s->parent()->indexOfChild(s));
                }
                if (!ignore) {
                    if (item == NULL) {
                        addTopLevelItem(s);
                    } else {
                        item->addChild(s);
                    }
                }
            }
            project_changed = true;
        }
    }
}
