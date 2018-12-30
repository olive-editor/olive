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
	ReplaceClipMediaDialog(QWidget* parent, Media *old_media);
private slots:
	void replace();
private:
	Media* media;
	QTreeView* tree;
	QCheckBox* use_same_media_in_points;
};

#endif // REPLACECLIPMEDIADIALOG_H
