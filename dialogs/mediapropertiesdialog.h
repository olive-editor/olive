#ifndef MEDIAPROPERTIESDIALOG_H
#define MEDIAPROPERTIESDIALOG_H

#include <QDialog>

struct Footage;
class QComboBox;
class QLineEdit;
class Media;

class MediaPropertiesDialog : public QDialog {
	Q_OBJECT
public:
    MediaPropertiesDialog(QWidget *parent, Media* i);
private:
	QComboBox* interlacing_box;
    QLineEdit* name_box;
    Media* item;
private slots:
    void accept();
};

#endif // MEDIAPROPERTIESDIALOG_H
