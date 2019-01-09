/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
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
