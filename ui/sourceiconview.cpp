#include "sourceiconview.h"

#include "panels/project.h"
#include "project/media.h"
#include "debug.h"

SourceIconView::SourceIconView(QWidget *parent) : QListView(parent) {}

void SourceIconView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (selectedIndexes().size() == 1) {
        Media* m = project_parent->item_to_media(selectedIndexes().at(0));
        if (m->get_type() == MEDIA_TYPE_FOLDER) {
            setRootIndex(selectedIndexes().at(0));
            emit changed_root();
        }
    }
}
