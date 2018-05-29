#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

namespace Ui {
class ExportDialog;
}

struct Sequence;

class ExportDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ExportDialog(QWidget *parent = 0);
	~ExportDialog();

	void set_defaults(Sequence* s);

private slots:
	void on_formatCombobox_currentIndexChanged(int index);

	void on_pushButton_2_clicked();

	void on_pushButton_clicked();

	void update_progress_bar(int value);

private:
	Ui::ExportDialog *ui;

	QVector<QString> format_strings;
	QVector<int> format_vcodecs;
	QVector<int> format_acodecs;
};

#endif // EXPORTDIALOG_H
