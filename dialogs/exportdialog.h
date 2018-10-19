#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

namespace Ui {
class ExportDialog;
}

struct Sequence;
class ExportThread;

class ExportDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ExportDialog(QWidget *parent = 0);
    ~ExportDialog();
    QString export_error;

private slots:
	void on_formatCombobox_currentIndexChanged(int index);

	void on_pushButton_2_clicked();

	void on_pushButton_clicked();

	void update_progress_bar(int value);

    void on_renderCancel_clicked();

    void render_thread_finished();

	void on_vcodecCombobox_currentIndexChanged(int index);

	void on_compressionTypeCombobox_currentIndexChanged(int index);

private:
	Ui::ExportDialog *ui;

	QVector<QString> format_strings;
	QVector<int> format_vcodecs;
	QVector<int> format_acodecs;

    ExportThread* et;
	void prep_ui_for_render(bool r);
    bool cancelled;
};

#endif // EXPORTDIALOG_H
