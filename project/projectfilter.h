#ifndef PROJECTFILTER_H
#define PROJECTFILTER_H

#include <QSortFilterProxyModel>

class ProjectFilter : public QSortFilterProxyModel {
	Q_OBJECT
public:
	ProjectFilter(QObject *parent = nullptr);

    // are sequences visible
	bool get_show_sequences();

public slots:

    // set whether sequences are visible
	void set_show_sequences(bool b);

    // update search filter
    void update_search_filter(const QString& s);

protected:

    // function that filters whether rows are displayed or not
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:

    // internal variable for whether to show sequences
	bool show_sequences;

    // search filter variable
    QString search_filter;

};

#endif // PROJECTFILTER_H
