#ifndef PROJECTFILTER_H
#define PROJECTFILTER_H

#include <QSortFilterProxyModel>

class ProjectFilter : public QSortFilterProxyModel {
	Q_OBJECT
public:
	ProjectFilter(QObject *parent = nullptr);
	bool get_show_sequences();
public slots:
	void set_show_sequences(bool b);
protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
private:
	bool show_sequences;
};

#endif // PROJECTFILTER_H
