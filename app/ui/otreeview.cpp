#include "otreeview.h"

OTreeView::OTreeView(QWidget *parent) : QTreeView(parent) {
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(show_context_menu()));
    connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(item_click(const QModelIndex&)));
}
