#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QKeySequenceEdit>
class QMenuBar;
class QLineEdit;
class QComboBox;
class QRadioButton;
class QTreeWidget;
class QTreeWidgetItem;
class QMenu;
class QCheckBox;
class QDoubleSpinBox;

class KeySequenceEditor : public QKeySequenceEdit {
	Q_OBJECT
public:
	KeySequenceEditor(QWidget *parent, QAction* a);
	void set_action_shortcut();
private:
	QAction* action;
};

class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PreferencesDialog(QWidget *parent = 0);
	~PreferencesDialog();

	void setup_kbd_shortcuts(QMenuBar* menu);

private slots:
	void save();
	void reset_default_shortcut();
	bool refine_shortcut_list(const QString &, QTreeWidgetItem* parent = NULL);

private:
	void setup_ui();
	void setup_kbd_shortcut_worker(QMenu* menu, QTreeWidgetItem* parent);

	QLineEdit* imgSeqFormatEdit;
	QComboBox* recordingComboBox;
	QRadioButton* accurateSeekButton;
	QRadioButton* fastSeekButton;
	QTreeWidget* keyboard_tree;
	QCheckBox* disable_img_multithread;
	QDoubleSpinBox* upcoming_queue_spinbox;
	QComboBox* upcoming_queue_type;
	QDoubleSpinBox* previous_queue_spinbox;
	QComboBox* previous_queue_type;

	QVector<QAction*> key_shortcut_actions;
	QVector<QTreeWidgetItem*> key_shortcut_items;
	QVector<KeySequenceEditor*> key_shortcut_fields;

	QPushButton* reset_shortcut_button;
};

#endif // PREFERENCESDIALOG_H
