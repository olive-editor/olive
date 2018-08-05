#ifndef REPLACECLIPMEDIADIALOG_H
#define REPLACECLIPMEDIADIALOG_H

#include <QDialog>

class SourceTable;
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;

class ReplaceClipMediaDialog : public QDialog {
	Q_OBJECT
public:
	ReplaceClipMediaDialog(QWidget* parent, SourceTable* table, QTreeWidgetItem *old_media);
private slots:
	void replace();
private:
	SourceTable* source_table;
	QTreeWidget* tree;
	QTreeWidgetItem* media;
	QCheckBox* use_same_media_in_points;
	void copy_tree(QTreeWidgetItem* parent, QTreeWidgetItem *target);
};

#endif // REPLACECLIPMEDIADIALOG_H
