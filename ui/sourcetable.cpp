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
    setAcceptDrops(true);
    editing_item = NULL;
    rename_timer.setInterval(500);
    connect(&rename_timer, SIGNAL(timeout()), this, SLOT(rename_interval()));
    connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(item_click(QTreeWidgetItem*,int)));
//    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(stop_rename_timer()));

//    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, )
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
	if (selectedItems().count() == 0) {
		panel_project->import_dialog();
	} else if (selectedItems().count() == 1) {
        QTreeWidgetItem* item = selectedItems().at(0);
        Media* m = reinterpret_cast<Media*>(item->data(0, Qt::UserRole + 1).value<quintptr>());
        if (m->type == MEDIA_TYPE_SEQUENCE) {
            set_sequence(m->sequence);
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
    }
}
