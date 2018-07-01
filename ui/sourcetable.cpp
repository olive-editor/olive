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
}

void SourceTable::mouseDoubleClickEvent(QMouseEvent* )
{
	if (selectedItems().count() == 0) {
		panel_project->import_dialog();
	} else if (selectedItems().count() == 1) {
		Media* m = reinterpret_cast<Media*>(selectedItems().at(0)->data(0, Qt::UserRole + 1).value<quintptr>());
		if (m->is_sequence) {
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
    QTreeWidget::dropEvent(event);
}
