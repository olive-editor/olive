#ifndef REPLACECLIPMEDIADIALOG_H
#define REPLACECLIPMEDIADIALOG_H

#include <QDialog>

class SourceTable;
class QTreeView;
class Media;
class QCheckBox;

class ReplaceClipMediaDialog : public QDialog {
	Q_OBJECT
public:
    ReplaceClipMediaDialog(QWidget* parent, SourceTable* table, Media *old_media);
private slots:
	void replace();
private:
	SourceTable* source_table;
    QTreeView* tree;
    Media* media;
    QCheckBox* use_same_media_in_points;
};

#endif // REPLACECLIPMEDIADIALOG_H
