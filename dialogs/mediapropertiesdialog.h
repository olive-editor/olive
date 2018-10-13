#ifndef MEDIAPROPERTIESDIALOG_H
#define MEDIAPROPERTIESDIALOG_H

#include <QDialog>

struct Media;
class QComboBox;
class QLineEdit;
class QTreeWidgetItem;

class MediaPropertiesDialog : public QDialog {
	Q_OBJECT
public:
    MediaPropertiesDialog(QWidget *parent, QTreeWidgetItem* i, Media *m);
private:
	QComboBox* interlacing_box;
    QLineEdit* name_box;
    QTreeWidgetItem* item;
    Media* media;
private slots:
    void accept();
};

#endif // MEDIAPROPERTIESDIALOG_H
