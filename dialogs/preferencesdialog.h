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
#ifndef NO_OCIO
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;
#endif

#include "timeline/sequence.h"

class KeySequenceEditor;

/**
 * @brief The PreferencesDialog class
 *
 * A dialog for the global application settings. Mostly an interface for Config.
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
   * @brief Delete all previews (waveform and thumbnail cache)
   */
  void delete_all_previews();

  // Browse for file functionns
  /**
   * @brief Show a file dialog to browse for an external CSS file to load for styling the application.
   */
  void browse_css_file();
  void browse_ocio_config();

  // OCIO function
#ifndef NO_OCIO
  void update_ocio_view_menu();
  void update_ocio_view_menu(OCIO::ConstConfigRcPtr config);
  void update_ocio_config(const QString&);
#endif

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

  enum PreviewDeleteTypes {
    DELETE_NONE,
    DELETE_THUMBNAILS,
    DELETE_WAVEFORMS,
    DELETE_BOTH
  };

  // used to delete previews
  // type can be: 't' for thumbnails, 'w' for waveforms, or 1 for all
  /**
   * @brief Delete disk cached preview files (thumbnails, waveforms, etc.)
   *
   * @param type
   *
   * The types of previews to delete.
   */
  void delete_previews(PreviewDeleteTypes type);

#ifndef NO_OCIO
  void populate_ocio_menus(OCIO::ConstConfigRcPtr config);
#endif

  QLineEdit* custom_css_fn;
  QLineEdit* imgSeqFormatEdit;
  QComboBox* recordingComboBox;
  QTreeWidget* keyboard_tree;
  QDoubleSpinBox* upcoming_queue_spinbox;
  QComboBox* upcoming_queue_type;
  QDoubleSpinBox* previous_queue_spinbox;
  QComboBox* previous_queue_type;
  QSpinBox* effect_textbox_lines_field;
  QCheckBox* use_software_fallbacks_checkbox;
  QComboBox* audio_output_devices;
  QComboBox* audio_input_devices;
  QComboBox* audio_sample_rate;
  QComboBox* language_combobox;
  QSpinBox* thumbnail_res_spinbox;
  QSpinBox* waveform_res_spinbox;
  QCheckBox* add_default_effects_to_clips;

  QCheckBox* enable_color_management;
  QLineEdit* ocio_config_file;
  QComboBox* ocio_display;
  QComboBox* ocio_view;
  QComboBox* ocio_look;

  QComboBox* ui_style;
  Sequence sequence_settings;
#ifdef Q_OS_WIN
  QCheckBox* native_menus;
#endif

  QVector<QAction*> key_shortcut_actions;
  QVector<QTreeWidgetItem*> key_shortcut_items;
  QVector<KeySequenceEditor*> key_shortcut_fields;
};

class KeySequenceEditor : public QKeySequenceEdit {
  Q_OBJECT
public:
  KeySequenceEditor(QWidget *parent, QAction* a);
  void set_action_shortcut();
  void reset_to_default();
  QString action_name();
  QString export_shortcut();
private:
  QAction* action;
};

#endif // PREFERENCESDIALOG_H
