#ifndef NEWSEQUENCEDIALOG_H
#define NEWSEQUENCEDIALOG_H

#include <QDialog>

class Project;

namespace Ui {
class NewSequenceDialog;
}

class NewSequenceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewSequenceDialog(QWidget *parent = 0);
	~NewSequenceDialog();
	void set_sequence_name(const QString& s);

private slots:
	void on_buttonBox_accepted();

	void on_comboBox_currentIndexChanged(int index);

private:
	Ui::NewSequenceDialog *ui;
};

#endif // NEWSEQUENCEDIALOG_H
