#ifndef MEDIAPROPERTIESDIALOG_H
#define MEDIAPROPERTIESDIALOG_H

#include <QDialog>

class QComboBox;

class MediaPropertiesDialog : public QDialog {
	Q_OBJECT
public:
	explicit MediaPropertiesDialog(QWidget *parent = 0);
private:
	QComboBox* interlacing_box;
};

#endif // MEDIAPROPERTIESDIALOG_H
