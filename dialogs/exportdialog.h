#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>

struct Sequence;
class ExportThread;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;
class QProgressBar;
class QGroupBox;

class ExportDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ExportDialog(QWidget *parent = 0);
	~ExportDialog();
	QString export_error;

private slots:
	void format_changed(int index);
	void export_action();
	void update_progress_bar(int value, qint64 remaining_ms);
	void cancel_render();
	void render_thread_finished();
	void vcodec_changed(int index);
	void comp_type_changed(int index);

private:
	QVector<QString> format_strings;
	QVector<int> format_vcodecs;
	QVector<int> format_acodecs;
	void setup_ui();

	ExportThread* et;
	void prep_ui_for_render(bool r);
	bool cancelled;

	QComboBox* rangeCombobox;
	QSpinBox* widthSpinbox;
	QDoubleSpinBox* videobitrateSpinbox;
	QLabel* videoBitrateLabel;
	QDoubleSpinBox* framerateSpinbox;
	QComboBox* vcodecCombobox;
	QComboBox* acodecCombobox;
	QSpinBox* samplingRateSpinbox;
	QSpinBox* audiobitrateSpinbox;
	QProgressBar* progressBar;
	QComboBox* formatCombobox;
	QSpinBox* heightSpinbox;
	QPushButton* export_button;
	QPushButton* cancel_button;
	QPushButton* renderCancel;
	QGroupBox* videoGroupbox;
	QGroupBox* audioGroupbox;
	QComboBox* compressionTypeCombobox;
};

#endif // EXPORTDIALOG_H
