#ifndef NEWSEQUENCEDIALOG_H
#define NEWSEQUENCEDIALOG_H

#include <QDialog>

class Project;
class Media;
class QComboBox;
class QSpinBox;
class QLineEdit;
struct Sequence;

class NewSequenceDialog : public QDialog
{
	Q_OBJECT

public:
	explicit NewSequenceDialog(QWidget *parent = 0, Media* existing = 0);
	~NewSequenceDialog();

	void set_sequence_name(const QString& s);

private slots:
	void create();
	void preset_changed(int index);

private:
	Sequence* existing_sequence;
	Media* existing_item;

	void setup_ui();

	QComboBox* preset_combobox;
	QSpinBox* height_numeric;
	QSpinBox* width_numeric;
	QComboBox* par_combobox;
	QComboBox* interlacing_combobox;
	QComboBox* frame_rate_combobox;
	QComboBox* audio_frequency_combobox;
    QLineEdit* sequence_name_edit;
};

#endif // NEWSEQUENCEDIALOG_H
