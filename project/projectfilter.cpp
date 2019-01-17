#include "projectfilter.h"

#include "project/media.h"

#include <QDebug>

ProjectFilter::ProjectFilter(QObject *parent) :
	QSortFilterProxyModel(parent),
	show_sequences(true)
{}

bool ProjectFilter::get_show_sequences() {
	return show_sequences;
}

void ProjectFilter::set_show_sequences(bool b) {
	show_sequences = b;
	invalidateFilter();
}

bool ProjectFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
	if (!show_sequences) {
		// hide sequences if show_sequences is false
		QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
		Media* media = static_cast<Media*>(index.internalPointer());
		if (media != nullptr && media->get_type() == MEDIA_TYPE_SEQUENCE) {
			return false;
		}
	}
	return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
