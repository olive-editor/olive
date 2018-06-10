#include "sourcetable.h"
#include "panels/project.h"

#include "io/media.h"
#include "panels/timeline.h"
#include "panels/viewer.h"
#include "panels/panels.h"

#include <QDragEnterEvent>

SourceTable::SourceTable(QWidget* parent) : QTreeWidget(parent) {
	this->sortByColumn(0, Qt::AscendingOrder);
}

void SourceTable::mouseDoubleClickEvent(QMouseEvent* )
{
	if (selectedItems().count() == 0) {
		panel_project->import_dialog();
	} else if (selectedItems().count() == 1) {
		Media* m = reinterpret_cast<Media*>(selectedItems().at(0)->data(0, Qt::UserRole + 1).value<quintptr>());
		if (m->is_sequence) {
            panel_project->set_sequence(m->sequence);
		}
	}
}

//void SourceTable::dragEnterEvent(QDragEnterEvent *event) {
//	event->accept();
//}
