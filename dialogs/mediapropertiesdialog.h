#ifndef MEDIAPROPERTIESDIALOG_H
#define MEDIAPROPERTIESDIALOG_H

#include <QDialog>

struct Footage;
class QComboBox;
class QLineEdit;
class Media;
class QListWidget;
class QDoubleSpinBox;

class MediaPropertiesDialog : public QDialog {
	Q_OBJECT
public:
	MediaPropertiesDialog(QWidget *parent, Media* i);
private:
	QComboBox* interlacing_box;
	QLineEdit* name_box;
	Media* item;
	QListWidget* track_list;
	QDoubleSpinBox* conform_fr;
private slots:
	void accept();
};

#endif // MEDIAPROPERTIESDIALOG_H
