/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QKeySequenceEdit>
#include <QMenuBar>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QTimeEdit>

#include "timeline/sequence.h"

class KeySequenceEditor;

/**
 * @brief The PreferencesDialog class
 *
 * A dialog for the global application settings. Mostly an interface for Config. Can be loaded from any part of the
 * application.
 */
class PreferencesDialog : public QDialog
{
  Q_OBJECT

public:
  /**
   * @brief PreferencesDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   */
  explicit PreferencesDialog(QWidget *parent = nullptr);

private slots:
  /**
   * @brief Override of accept to save preferences to Config.
   */
  virtual void accept() override;

  /**
   * @brief Reset all selected shortcuts in keyboard_tree to their defaults
   */
  void reset_default_shortcut();

  /**
   * @brief Reset all shortcuts indiscriminately to their defaults
   *
   * This is safe to call directly as it'll ask the user if they wish to do so before it resets.
   */
  void reset_all_shortcuts();

  /**
   * @brief Shows/hides shortcut entries according to a shortcut query.
   *
   * This function can be directly connected to QLineEdit::textChanged() for simplicity.
   *
   * @param s
   *
   * The search query to compare shortcut names to.
   *
   * @param parent
   *
   * This is used as the function calls itself recursively to traverse the menu item hierarchy. This should be left as
   * nullptr when called externally.
   *
   * @return
   *
   * Value used as function calls itself recursively to determine if a menu parent has any children that are not hidden.
   * If so, TRUE is returned so the parent is shown too (even if it doesn't match the search query). If not, FALSE is
   * returned so the parent is hidden.
   */
  bool refine_shortcut_list(const QString &s, QTreeWidgetItem* parent = nullptr);

  /**
   * @brief Show a file dialog to load an external shortcut preset from file
   */
  void load_shortcut_file();

  /**
   * @brief Show a file dialog to save an external shortcut preset from file
   */
  void save_shortcut_file();

  /**
   * @brief Show a file dialog to browse for an external CSS file to load for styling the application.
   */
  void browse_css_file();

  /**
   * @brief Delete all previews (waveform and thumbnail cache)
   */
  void delete_all_previews();

  /**
   * @brief Shows a NewSequenceDialog attached to default_sequence
   */
  void edit_default_sequence_settings();

private:

  /**
   * @brief Create and arrange all UI widgets
   */
  void setup_ui();

  /**
   * @brief Populate keyboard shortcut panel with keyboard shortcuts from the menu bar
   *
   * @param menu
   *
   * A reference to the main application's menu bar. Usually MainWindow::menuBar().
   */
  void setup_kbd_shortcuts(QMenuBar* menu);

  /**
   * @brief Internal function called by setup_kbd_shortcuts() to traverse down the menu bar's hierarchy and populate the
   * shortcut panel.
   *
   * This function will call itself recursively as it finds submenus belong to the menu provided. It will also create
   * QTreeWidgetItems as children of the parent item provided, either using them as parents themselves for submenus
   * or attaching a KeySequenceEditor to them for shortcut editing.
   *
   * @param menu
   *
   * The current menu to traverse down.
   *
   * @param parent
   *
   * The parent item to add QTreeWidgetItems to.
   */
  void setup_kbd_shortcut_worker(QMenu* menu, QTreeWidgetItem* parent);

  // used to delete previews
  // type can be: 't' for thumbnails, 'w' for waveforms, or 1 for all
  /**
   * @brief Delete disk cached preview files (thumbnails, waveforms, etc.)
   *
   * @param type
   *
   * The types of previews to
   */
  void delete_previews(char type);

  /**
   * @brief UI widget for editing the CSS filename
   */
  QLineEdit* custom_css_fn;

  /**
   * @brief UI widget for editing the list of extensions to detect image sequences from
   */
  QLineEdit* imgSeqFormatEdit;

  /**
   * @brief UI widget for editing the recording channels
   */
  QComboBox* recordingComboBox;

  /**
   * @brief UI widget for editing keyboard shortcuts
   */
  QTreeWidget* keyboard_tree;

  /**
   * @brief UI widget for editing the upcoming queue size
   */
  QDoubleSpinBox* upcoming_queue_spinbox;

  /**
   * @brief UI widget for editing the upcoming queue type
   */
  QComboBox* upcoming_queue_type;

  /**
   * @brief UI widget for editing the previous queue size
   */
  QDoubleSpinBox* previous_queue_spinbox;

  /**
   * @brief UI widget for editing the previous queue type
   */
  QComboBox* previous_queue_type;

  /**
   * @brief UI widget for editing the size of textboxes in the EffectControls panel
   */
  QSpinBox* effect_textbox_lines_field;

  /**
   * @brief UI widget for selecting the output audio device
   */
  QComboBox* audio_output_devices;

  /**
   * @brief UI widget for selecting the input audio device
   */
  QComboBox* audio_input_devices;

  /**
   * @brief UI widget for selecting the audio sampling rates
   */
  QComboBox* audio_sample_rate;

  /**
   * @brief UI widget for selecting the UI language
   */
  QComboBox* language_combobox;

  /**
   * @brief UI widget for selecting the resolution of the thumbnails to generate
   */
  QSpinBox* thumbnail_res_spinbox;

  /**
   * @brief UI widget for selecting the resolution of the waveforms to generate
   */
  QSpinBox* waveform_res_spinbox;

  /**
   * @brief UI widget for selecting the default length of an image footage
   */
  QTimeEdit* default_image_duration_timeedit;

  /**
   * @brief UI widget for selecting the current UI style
   */
  QComboBox* ui_style;

  /**
   * @brief Stored default Sequence object
   *
   * Default Sequence settings are loaded into an actual Sequence object that can be loaded into NewSequenceDialog
   * for the sake of familiarity with the user.
   */
  Sequence default_sequence;

  /**
   * @brief List of keyboard shortcut actions that can be triggered (links with key_shortcut_items and
   * key_shortcut_fields)
   */
  QVector<QAction*> key_shortcut_actions;

  /**
   * @brief List of keyboard shortcut items in keyboard_tree corresponding to existing actions (links with
   * key_shortcut_actions and key_shortcut_fields)
   */
  QVector<QTreeWidgetItem*> key_shortcut_items;

  /**
   * @brief List of keyboard shortcut editing fields in keyboard_tree corresponding to existing actions (links with
   * key_shortcut_actions and key_shortcut_fields)
   */
  QVector<KeySequenceEditor*> key_shortcut_fields;

  /**
   * @brief Add an automated QCheckBox+boolean value pair
   *
   * Many preferences are simple true/false (or on/off) options. Rather than adding a QCheckBox for each one and
   * manually setting its checked value to the configuration setting (and vice versa when saving), this convenience
   * function will add it to an automated set of checkboxes, automatically setting the checked state to the current
   * setting, and then saving the new checked state back to the setting when the user accepts the changes (clicks OK).
   *
   * @param ui
   *
   * A valid QCheckBox item. This function does not take ownership of the QWidget or place it in a layout anywhere.
   *
   * @param value
   *
   * A pointer to the Boolean value this QCheckBox should be shared with. The QCheckBox widget's checked state will be
   * set to the value of this pointer.
   *
   * @param restart_required
   *
   * Defaults to FALSE, set this to TRUE if changing this setting should prompt the user for a restart of Olive before
   * the setting change takes effect.
   */
  void AddBoolPair(QCheckBox* ui, bool* value, bool restart_required = false);

  /**
   * @brief Internal array managed by AddBoolPair(). Do not access this directly.
   */
  QVector<QCheckBox*> bool_ui;

  /**
   * @brief Internal array managed by AddBoolPair(). Do not access this directly.
   */
  QVector<bool*> bool_value;

  /**
   * @brief Internal array managed by AddBoolPair(). Do not access this directly.
   */
  QVector<bool> bool_restart_required;
};

/**
 * @brief The KeySequenceEditor class
 *
 * Simple derived class of QKeySequenceEdit that attaches to a QAction and provides functions for transferring
 * keyboard shortcuts to and from it.
 */
class KeySequenceEditor : public QKeySequenceEdit {
  Q_OBJECT
public:
  /**
   * @brief KeySequenceEditor Constructor
   *
   * @param parent
   *
   * QWidget parent.
   *
   * @param a
   *
   * The QAction to link to. This cannot be changed throughout the lifetime of a KeySequenceEditor.
   */
  KeySequenceEditor(QWidget *parent, QAction* a);

  /**
   * @brief Sets the attached QAction's shortcut to the shortcut entered in this field.
   *
   * This is not done automatically in case the user cancels out of the Preferences dialog, in which case the
   * expectation is that the changes made will not be saved. Therefore, this needs to be triggered manually when
   * PreferencesDialog saves.
   */
  void set_action_shortcut();

  /**
   * @brief Set this shortcut back to the QAction's default shortcut
   *
   * Each QAction contains the default shortcut in its `property("default")` and can be used to restore the default
   * "hard-coded" shortcut with this function.
   *
   * This function does not save the default shortcut back into the QAction, it simply loads the default shortcut from
   * the QAction into this edit field. To save it into the QAction, it's necessary to call set_action_shortcut() after
   * calling this function.
   */
  void reset_to_default();

  /**
   * @brief Return attached QAction's unique ID
   *
   * Each of Olive's menu actions has a unique string ID (that, unlike the text, is not translated) for matching with
   * an external shortcut configuration file. The ID is stored in the QAction's `property("id")`. This function returns
   * that ID.
   *
   * @return
   *
   * The QAction's unique ID.
   */
  QString action_name();

  /**
   * @brief Serialize this shortcut entry into a string that can be saved to a file
   *
   * @return
   *
   * A string serialization of this shortcut. The format is "[ID]\t[SEQUENCE]" where [ID] is the attached QAction's
   * unique identifier and [SEQUENCE] is the current keyboard shortcut in the field (NOT necessarily the shortcut in
   * the QAction). If the entered shortcut is the same as the QAction's default shortcut, the return value is empty
   * because a default shortcut does not need to be saved to a file.
   */
  QString export_shortcut();
private:
  /**
   * @brief Internal reference to the linked QAction
   */
  QAction* action;
};

#endif // PREFERENCESDIALOG_H
