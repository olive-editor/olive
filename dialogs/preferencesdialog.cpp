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

#include "preferencesdialog.h"

#include "global/global.h"
#include "global/config.h"
#include "global/path.h"
#include "rendering/audio.h"
#include "panels/panels.h"
#include "ui/mainwindow.h"

#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QVector>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QList>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QAudioDeviceInfo>
#include <QApplication>
#include <QProcess>
#include <QDebug>

KeySequenceEditor::KeySequenceEditor(QWidget* parent, QAction* a)
  : QKeySequenceEdit(parent), action(a) {
  setKeySequence(action->shortcut());
}

void KeySequenceEditor::set_action_shortcut() {
  action->setShortcut(keySequence());
  action->setShortcutContext(Qt::ApplicationShortcut);
}

void KeySequenceEditor::reset_to_default() {
  setKeySequence(action->property("default").toString());
}

QString KeySequenceEditor::action_name() {
  return action->property("id").toString();
}

QString KeySequenceEditor::export_shortcut() {
  QString ks = keySequence().toString();
  if (ks != action->property("default")) {
    return action->property("id").toString() + "\t" + keySequence().toString();
  }
  return nullptr;
}

PreferencesDialog::PreferencesDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowTitle(tr("Preferences"));
  setup_ui();

  accurateSeekButton->setChecked(!olive::CurrentConfig.fast_seeking);
  fastSeekButton->setChecked(olive::CurrentConfig.fast_seeking);
  recordingComboBox->setCurrentIndex(olive::CurrentConfig.recording_mode - 1);
  imgSeqFormatEdit->setText(olive::CurrentConfig.img_seq_formats);
}

PreferencesDialog::~PreferencesDialog() {}

void PreferencesDialog::setup_kbd_shortcut_worker(QMenu* menu, QTreeWidgetItem* parent) {
  QList<QAction*> actions = menu->actions();
  for (int i=0;i<actions.size();i++) {
    QAction* a = actions.at(i);

    if (!a->isSeparator() && a->property("keyignore").isNull()) {
      QTreeWidgetItem* item = new QTreeWidgetItem(parent);
      item->setText(0, a->text().replace("&", ""));

      parent->addChild(item);

      if (a->menu() != nullptr) {
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        setup_kbd_shortcut_worker(a->menu(), item);
      } else {
        key_shortcut_items.append(item);
        key_shortcut_actions.append(a);
      }
    }
  }
}

void PreferencesDialog::delete_previews(char type) {
  if (type != 't' && type != 'w' && type != 1) return;

  QDir preview_path(get_data_path() + "/previews");

  if (type == 1) {
    // indiscriminately delete everything
    preview_path.removeRecursively();
  } else {
    QStringList preview_file_list = preview_path.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (int i=0;i<preview_file_list.size();i++) {

      const QString& preview_file_str = preview_file_list.at(i);

      // use filename to determine whether this is a thumbnail or a waveform
      int identifier_char_index = qMax(0, preview_file_str.size()-2);

      // find identifier char
      while (identifier_char_index >= 0
             && preview_file_str.at(identifier_char_index) >= 48
             && preview_file_str.at(identifier_char_index) <= 57) {
        identifier_char_index--;
      }

      // thumbnails will have a 't' towards the end of the filenames, waveforms will have a 'w'
      // if they match the type of preview we're deleting, remove them
      if (preview_file_str.at(identifier_char_index) == type) {
        QFile::remove(preview_path.filePath(preview_file_str));
      }
    }
  }
}

void PreferencesDialog::setup_kbd_shortcuts(QMenuBar* menubar) {
  QList<QAction*> menus = menubar->actions();

  for (int i=0;i<menus.size();i++) {
    QMenu* menu = menus.at(i)->menu();

    QTreeWidgetItem* item = new QTreeWidgetItem(keyboard_tree);
    item->setText(0, menu->title().replace("&", ""));

    keyboard_tree->addTopLevelItem(item);

    setup_kbd_shortcut_worker(menu, item);
  }

  for (int i=0;i<key_shortcut_items.size();i++) {
    if (!key_shortcut_actions.at(i)->property("id").isNull()) {
      KeySequenceEditor* editor = new KeySequenceEditor(keyboard_tree, key_shortcut_actions.at(i));
      keyboard_tree->setItemWidget(key_shortcut_items.at(i), 1, editor);
      key_shortcut_fields.append(editor);
    }
  }
}

void PreferencesDialog::save() {
  bool restart_after_saving = false;
  bool reinit_audio = false;
  bool reload_language = false;
  bool reload_effects = false;

  // Validate whether the specified CSS file exists
  if (!custom_css_fn->text().isEmpty() && !QFileInfo::exists(custom_css_fn->text())) {
    QMessageBox::critical(
          this,
          tr("Invalid CSS File"),
          tr("CSS file '%1' does not exist.").arg(custom_css_fn->text())
          );
    return;
  }

  // Validate whether the effects panel should refresh itself
  if (olive::CurrentConfig.effect_textbox_lines != effect_textbox_lines_field->value()) {
    reload_effects = true;
  }

  // Check if any settings will require a restart of Olive
  if (olive::CurrentConfig.use_software_fallback != use_software_fallbacks_checkbox->isChecked()
      || olive::CurrentConfig.thumbnail_resolution != thumbnail_res_spinbox->value()
      || olive::CurrentConfig.waveform_resolution != waveform_res_spinbox->value()
#ifdef Q_OS_WIN32
      || olive::CurrentConfig.use_native_menu_styling != native_menus->isChecked()
#endif
      || olive::CurrentConfig.style != static_cast<olive::styling::Style>(ui_style->currentData().toInt())) {

    // any changes to these settings will require a restart - ask the user if we should do one now or later

    int ret = QMessageBox::question(this,
                                    "Restart Required",
                                    "Some of the changed settings will require a restart of Olive. Would you like "
                                    "to restart now?",
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (ret == QMessageBox::Cancel) {
      // Return to Preferences dialog without saving any settings
      return;
    } else if (ret == QMessageBox::Yes) {

      // Check if we can close the current project. If not, we'll treat it as if the user clicked "Cancel".
      if (olive::Global->can_close_project()) {
        restart_after_saving = true;
      } else {
        return;
      }
    }
    // Selecting "No" will save the settings and not restart. They will become active next time Olive opens.

  }

  // Audio settings may require the audio device to be re-initiated.
  if (olive::CurrentConfig.preferred_audio_output != audio_output_devices->currentData().toString()
      || olive::CurrentConfig.preferred_audio_input != audio_input_devices->currentData().toString()
      || olive::CurrentConfig.audio_rate != audio_sample_rate->currentData().toInt()) {
    reinit_audio = true;
  }

  // see if the language file should be reloaded (not necessary if the app is restarting anyway)
  if (!restart_after_saving
      && olive::CurrentConfig.language_file != language_combobox->currentData().toString()) {
    reload_language = true;
  }

  // save settings from UI to backend
  if (olive::CurrentConfig.css_path != custom_css_fn->text()) {
    olive::CurrentConfig.css_path = custom_css_fn->text();
    olive::MainWindow->Restyle();
  }

  olive::CurrentConfig.recording_mode = recordingComboBox->currentIndex() + 1;
  olive::CurrentConfig.img_seq_formats = imgSeqFormatEdit->text();
  olive::CurrentConfig.fast_seeking = fastSeekButton->isChecked();
  olive::CurrentConfig.upcoming_queue_size = upcoming_queue_spinbox->value();
  olive::CurrentConfig.upcoming_queue_type = upcoming_queue_type->currentIndex();
  olive::CurrentConfig.previous_queue_size = previous_queue_spinbox->value();
  olive::CurrentConfig.previous_queue_type = previous_queue_type->currentIndex();
  olive::CurrentConfig.add_default_effects_to_clips = add_default_effects_to_clips->isChecked();

  olive::CurrentConfig.preferred_audio_output = audio_output_devices->currentData().toString();
  olive::CurrentConfig.preferred_audio_input = audio_input_devices->currentData().toString();
  olive::CurrentConfig.audio_rate = audio_sample_rate->currentData().toInt();

  olive::CurrentConfig.effect_textbox_lines = effect_textbox_lines_field->value();
  olive::CurrentConfig.use_software_fallback = use_software_fallbacks_checkbox->isChecked();
  olive::CurrentConfig.language_file = language_combobox->currentData().toString();

  olive::CurrentConfig.style = static_cast<olive::styling::Style>(ui_style->currentData().toInt());
  olive::CurrentConfig.use_native_menu_styling = native_menus->isChecked();

  // Check if the thumbnail or waveform icon
  if (olive::CurrentConfig.thumbnail_resolution != thumbnail_res_spinbox->value()
      || olive::CurrentConfig.waveform_resolution != waveform_res_spinbox->value()) {
    // we're changing the size of thumbnails and waveforms, so let's delete them and regenerate them next start

    // delete nothing
    char delete_match = 0;

    if (olive::CurrentConfig.thumbnail_resolution != thumbnail_res_spinbox->value()) {
      // delete existing thumbnails
      olive::CurrentConfig.thumbnail_resolution = thumbnail_res_spinbox->value();

      // delete only thumbnails
      delete_match = 't';
    }

    if (olive::CurrentConfig.waveform_resolution != waveform_res_spinbox->value()) {
      // delete existing waveforms
      olive::CurrentConfig.waveform_resolution = waveform_res_spinbox->value();

      // if we're already deleting thumbnails
      if (delete_match == 't') {
        // delete all
        delete_match = 1;
      } else {
        // just delete waveforms
        delete_match = 'w';
      }
    }

    delete_previews(delete_match);
  }

  // Save keyboard shortcuts
  for (int i=0;i<key_shortcut_fields.size();i++) {
    key_shortcut_fields.at(i)->set_action_shortcut();
  }

  // Audio settings may require the audio device to be re-initiated.
  if (reinit_audio) {
    init_audio();
  }

  if (reload_effects) {
    panel_effect_controls->Reload();
  }

  // reload language file if it changed
  if (reload_language) {
    olive::Global->load_translation_from_config();
  }

  accept();

  if (restart_after_saving) {
    // since we already ran can_close_project(), bypass checking again by running setWindowModified(false)
    olive::MainWindow->setWindowModified(false);

    olive::MainWindow->close();

    QProcess::startDetached(QApplication::applicationFilePath(), { olive::ActiveProjectFilename });
  }
}

void PreferencesDialog::reset_default_shortcut() {
  QList<QTreeWidgetItem*> items = keyboard_tree->selectedItems();
  for (int i=0;i<items.size();i++) {
    QTreeWidgetItem* item = keyboard_tree->selectedItems().at(i);
    static_cast<KeySequenceEditor*>(keyboard_tree->itemWidget(item, 1))->reset_to_default();
  }
}

void PreferencesDialog::reset_all_shortcuts() {
  if (QMessageBox::question(
        this,
        tr("Confirm Reset All Shortcuts"),
        tr("Are you sure you wish to reset all keyboard shortcuts to their defaults?"),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    for (int i=0;i<key_shortcut_fields.size();i++) {
      key_shortcut_fields.at(i)->reset_to_default();
    }
  }
}

bool PreferencesDialog::refine_shortcut_list(const QString &s, QTreeWidgetItem* parent) {
  if (parent == nullptr) {
    for (int i=0;i<keyboard_tree->topLevelItemCount();i++) {
      refine_shortcut_list(s, keyboard_tree->topLevelItem(i));
    }
  } else {
    parent->setExpanded(!s.isEmpty());

    bool all_children_are_hidden = !s.isEmpty();

    for (int i=0;i<parent->childCount();i++) {
      QTreeWidgetItem* item = parent->child(i);
      if (item->childCount() > 0) {
        all_children_are_hidden = refine_shortcut_list(s, item);
      } else {
        item->setHidden(false);
        if (s.isEmpty()) {
          all_children_are_hidden = false;
        } else {
          QString shortcut;
          if (keyboard_tree->itemWidget(item, 1) != nullptr) {
            shortcut = static_cast<QKeySequenceEdit*>(keyboard_tree->itemWidget(item, 1))->keySequence().toString();
          }
          if (item->text(0).contains(s, Qt::CaseInsensitive) || shortcut.contains(s, Qt::CaseInsensitive)) {
            all_children_are_hidden = false;
          } else {
            item->setHidden(true);
          }
        }
      }
    }

    if (parent->text(0).contains(s, Qt::CaseInsensitive)) all_children_are_hidden = false;

    parent->setHidden(all_children_are_hidden);

    return all_children_are_hidden;
  }
  return true;
}

void PreferencesDialog::load_shortcut_file() {
  QString fn = QFileDialog::getOpenFileName(this, tr("Import Keyboard Shortcuts"));
  if (!fn.isEmpty()) {
    QFile f(fn);
    if (f.exists() && f.open(QFile::ReadOnly)) {
      QByteArray ba = f.readAll();
      f.close();
      for (int i=0;i<key_shortcut_fields.size();i++) {
        int index = ba.indexOf(key_shortcut_fields.at(i)->action_name());
        if (index == 0 || (index > 0 && ba.at(index-1) == '\n')) {
          while (index < ba.size() && ba.at(index) != '\t') index++;
          QString ks;
          index++;
          while (index < ba.size() && ba.at(index) != '\n') {
            ks.append(ba.at(index));
            index++;
          }
          key_shortcut_fields.at(i)->setKeySequence(ks);
        } else {
          key_shortcut_fields.at(i)->reset_to_default();
        }
      }
    } else {
      QMessageBox::critical(
            this,
            tr("Error saving shortcuts"),
            tr("Failed to open file for reading")
            );
    }
  }
}

void PreferencesDialog::save_shortcut_file() {
  QString fn = QFileDialog::getSaveFileName(this, tr("Export Keyboard Shortcuts"));
  if (!fn.isEmpty()) {
    QFile f(fn);
    if (f.open(QFile::WriteOnly)) {
      bool start = true;
      for (int i=0;i<key_shortcut_fields.size();i++) {
        QString s = key_shortcut_fields.at(i)->export_shortcut();
        if (!s.isEmpty()) {
          if (!start) f.write("\n");
          f.write(s.toUtf8());
          start = false;
        }
      }
      f.close();
      QMessageBox::information(this, tr("Export Shortcuts"), tr("Shortcuts exported successfully"));
    } else {
      QMessageBox::critical(this, tr("Error saving shortcuts"), tr("Failed to open file for writing"));
    }
  }
}

void PreferencesDialog::browse_css_file() {
  QString fn = QFileDialog::getOpenFileName(this, tr("Browse for CSS file"));
  if (!fn.isEmpty()) {
    custom_css_fn->setText(fn);
  }
}

void PreferencesDialog::delete_all_previews() {
  if (QMessageBox::question(this,
                            tr("Delete All Previews"),
                            tr("Are you sure you want to delete all previews?"),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    delete_previews(1);
    QMessageBox::information(this,
                             tr("Previews Deleted"),
                             tr("All previews deleted succesfully. You may have to re-open your current project for changes to take effect."),
                             QMessageBox::Ok);
  }
}

void PreferencesDialog::setup_ui() {
  QVBoxLayout* verticalLayout = new QVBoxLayout(this);
  QTabWidget* tabWidget = new QTabWidget(this);

  // row counter used to ease adding new rows
  int row = 0;

  // General
  QWidget* general_tab = new QWidget(this);
  QGridLayout* general_layout = new QGridLayout(general_tab);

  // General -> Language
  general_layout->addWidget(new QLabel(tr("Language:")), row, 0);

  language_combobox = new QComboBox();

  // add default language (en-US)
  language_combobox->addItem(QLocale::languageToString(QLocale("en-US").language()));

  // add languages from file
  QList<QString> translation_paths = get_language_paths();

  // iterate through all language search paths
  for (int j=0;j<translation_paths.size();j++) {
    QDir translation_dir(translation_paths.at(j));
    if (translation_dir.exists()) {
      QStringList translation_files = translation_dir.entryList({"*.qm"}, QDir::Files | QDir::NoDotAndDotDot);
      for (int i=0;i<translation_files.size();i++) {
        // get path of translation relative to the application path
        QString locale_full_path = translation_dir.filePath(translation_files.at(i));
        QString locale_relative_path = QDir(get_app_path()).relativeFilePath(locale_full_path);

        QFileInfo locale_file(translation_files.at(i));
        QString locale_file_basename = locale_file.baseName();
        QString locale_str = locale_file_basename.mid(locale_file_basename.lastIndexOf('_')+1);
        language_combobox->addItem(QLocale(locale_str).nativeLanguageName(), locale_relative_path);

        if (olive::CurrentConfig.language_file == locale_relative_path) {
          language_combobox->setCurrentIndex(language_combobox->count() - 1);
        }
      }
    }
  }

  general_layout->addWidget(language_combobox, row, 1, 1, 4);

  row++;

  // General -> Image Sequence Formats
  general_layout->addWidget(new QLabel(tr("Image sequence formats:"), this), row, 0);

  imgSeqFormatEdit = new QLineEdit(general_tab);

  general_layout->addWidget(imgSeqFormatEdit, row, 1, 1, 4);

  row++;

  // General -> Thumbnail and Waveform Resolution
  general_layout->addWidget(new QLabel(tr("Thumbnail Resolution:"), this), row, 0);

  thumbnail_res_spinbox = new QSpinBox(this);
  thumbnail_res_spinbox->setMinimum(0);
  thumbnail_res_spinbox->setMaximum(INT_MAX);
  thumbnail_res_spinbox->setValue(olive::CurrentConfig.thumbnail_resolution);
  general_layout->addWidget(thumbnail_res_spinbox, row, 1);

  general_layout->addWidget(new QLabel(tr("Waveform Resolution:"), this), row, 2);

  waveform_res_spinbox = new QSpinBox(this);
  waveform_res_spinbox->setMinimum(0);
  waveform_res_spinbox->setMaximum(INT_MAX);
  waveform_res_spinbox->setValue(olive::CurrentConfig.waveform_resolution);
  general_layout->addWidget(waveform_res_spinbox, row, 3);

  QPushButton* delete_preview_btn = new QPushButton(tr("Delete Previews"));
  general_layout->addWidget(delete_preview_btn, row, 4);
  connect(delete_preview_btn, SIGNAL(clicked(bool)), this, SLOT(delete_all_previews()));

  row++;

  // General -> Use Software Fallbacks When Possible
  use_software_fallbacks_checkbox = new QCheckBox(general_tab);
  use_software_fallbacks_checkbox->setText(tr("Use Software Fallbacks When Possible"));
  use_software_fallbacks_checkbox->setChecked(olive::CurrentConfig.use_software_fallback);
  general_layout->addWidget(use_software_fallbacks_checkbox, row, 0, 1, 4);

  row++;

  // General -> Default Sequence Settings
  QPushButton* default_sequence_settings = new QPushButton(tr("Default Sequence Settings"));
  general_layout->addWidget(default_sequence_settings);

  tabWidget->addTab(general_tab, tr("General"));

  // Behavior
  QWidget* behavior_tab = new QWidget(this);
  tabWidget->addTab(behavior_tab, tr("Behavior"));

  QVBoxLayout* behavior_tab_layout = new QVBoxLayout(behavior_tab);

  add_default_effects_to_clips = new QCheckBox("Add Default Effects to New Clips");
  add_default_effects_to_clips->setChecked(olive::CurrentConfig.add_default_effects_to_clips);
  behavior_tab_layout->addWidget(add_default_effects_to_clips);

  // Appearance
  QWidget* appearance_tab = new QWidget(this);
  tabWidget->addTab(appearance_tab, tr("Appearance"));

  row = 0;

  QGridLayout* appearance_layout = new QGridLayout(appearance_tab);

  // Appearance -> Theme
  appearance_layout->addWidget(new QLabel(tr("Theme")), row, 0);

  ui_style = new QComboBox();
  ui_style->addItem(tr("Olive Dark (Default)"), olive::styling::kOliveDefaultDark);
  ui_style->addItem(tr("Olive Light"), olive::styling::kOliveDefaultLight);
  ui_style->addItem(tr("Native"), olive::styling::kNativeDarkIcons);
  ui_style->addItem(tr("Native (Light Icons)"), olive::styling::kNativeLightIcons);
  ui_style->setCurrentIndex(olive::CurrentConfig.style);
  appearance_layout->addWidget(ui_style, row, 1, 1, 2);

  row++;

#ifdef Q_OS_WIN
  // Native menu styling is only available on Windows. Environments like Ubuntu and Mac use the native menu system by
  // default
  native_menus = new QCheckBox(tr("Use Native Menu Styling"));
  native_menus->setChecked(olive::CurrentConfig.use_native_menu_styling);
  appearance_layout->addWidget(native_menus, row, 0, 1, 3);

  row++;
#endif

  // Appearance -> Custom CSS
  appearance_layout->addWidget(new QLabel(tr("Custom CSS:"), this), row, 0);

  custom_css_fn = new QLineEdit(general_tab);
  custom_css_fn->setText(olive::CurrentConfig.css_path);
  appearance_layout->addWidget(custom_css_fn, row, 1);

  QPushButton* custom_css_browse = new QPushButton(tr("Browse"), general_tab);
  connect(custom_css_browse, SIGNAL(clicked(bool)), this, SLOT(browse_css_file()));
  appearance_layout->addWidget(custom_css_browse, row, 2);

  row++;

  // Appearance -> Effect Textbox Lines
  appearance_layout->addWidget(new QLabel(tr("Effect Textbox Lines:"), this), row, 0);

  effect_textbox_lines_field = new QSpinBox(general_tab);
  effect_textbox_lines_field->setMinimum(1);
  effect_textbox_lines_field->setValue(olive::CurrentConfig.effect_textbox_lines);
  appearance_layout->addWidget(effect_textbox_lines_field, row, 1, 1, 2);

  row++;

  // Playback
  QWidget* playback_tab = new QWidget(this);
  QVBoxLayout* playback_tab_layout = new QVBoxLayout(playback_tab);

  // Playback -> Seeking
  QGroupBox* seeking_group = new QGroupBox(playback_tab);
  seeking_group->setTitle(tr("Seeking"));
  QVBoxLayout* seeking_group_layout = new QVBoxLayout(seeking_group);
  accurateSeekButton = new QRadioButton(seeking_group);
  accurateSeekButton->setText(tr("Accurate Seeking\nAlways show the correct frame (visual may pause briefly as correct frame is retrieved)"));
  seeking_group_layout->addWidget(accurateSeekButton);
  fastSeekButton = new QRadioButton(seeking_group);
  fastSeekButton->setText(tr("Fast Seeking\nSeek quickly (may briefly show inaccurate frames when seeking - doesn't affect playback/export)"));
  seeking_group_layout->addWidget(fastSeekButton);
  playback_tab_layout->addWidget(seeking_group);

  // Playback -> Memory Usage
  QGroupBox* memory_usage_group = new QGroupBox(playback_tab);
  memory_usage_group->setTitle(tr("Memory Usage"));
  QGridLayout* memory_usage_layout = new QGridLayout(memory_usage_group);
  memory_usage_layout->addWidget(new QLabel(tr("Upcoming Frame Queue:"), playback_tab), 0, 0);
  upcoming_queue_spinbox = new QDoubleSpinBox(playback_tab);
  upcoming_queue_spinbox->setValue(olive::CurrentConfig.upcoming_queue_size);
  memory_usage_layout->addWidget(upcoming_queue_spinbox, 0, 1);
  upcoming_queue_type = new QComboBox(playback_tab);
  upcoming_queue_type->addItem(tr("frames"));
  upcoming_queue_type->addItem(tr("seconds"));
  upcoming_queue_type->setCurrentIndex(olive::CurrentConfig.upcoming_queue_type);
  memory_usage_layout->addWidget(upcoming_queue_type, 0, 2);
  memory_usage_layout->addWidget(new QLabel(tr("Previous Frame Queue:"), playback_tab), 1, 0);
  previous_queue_spinbox = new QDoubleSpinBox(playback_tab);
  previous_queue_spinbox->setValue(olive::CurrentConfig.previous_queue_size);
  memory_usage_layout->addWidget(previous_queue_spinbox, 1, 1);
  previous_queue_type = new QComboBox(playback_tab);
  previous_queue_type->addItem(tr("frames"));
  previous_queue_type->addItem(tr("seconds"));
  previous_queue_type->setCurrentIndex(olive::CurrentConfig.previous_queue_type);
  memory_usage_layout->addWidget(previous_queue_type, 1, 2);
  playback_tab_layout->addWidget(memory_usage_group);

  tabWidget->addTab(playback_tab, tr("Playback"));

  // Audio
  QWidget* audio_tab = new QWidget(this);

  QGridLayout* audio_tab_layout = new QGridLayout(audio_tab);

  row = 0;

  // Audio -> Output Device

  audio_tab_layout->addWidget(new QLabel(tr("Output Device:")), row, 0);

  audio_output_devices = new QComboBox();
  audio_output_devices->addItem(tr("Default"), "");

  // list all available audio output devices
  QList<QAudioDeviceInfo> devs = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
  bool found_preferred_device = false;
  for (int i=0;i<devs.size();i++) {
    audio_output_devices->addItem(devs.at(i).deviceName(), devs.at(i).deviceName());
    if (!found_preferred_device
        && devs.at(i).deviceName() == olive::CurrentConfig.preferred_audio_output) {
      audio_output_devices->setCurrentIndex(audio_output_devices->count()-1);
      found_preferred_device = true;
    }
  }

  audio_tab_layout->addWidget(audio_output_devices, row, 1);

  row++;

  // Audio -> Input Device

  audio_tab_layout->addWidget(new QLabel(tr("Input Device:")), row, 0);

  audio_input_devices = new QComboBox();
  audio_input_devices->addItem(tr("Default"), "");

  // list all available audio input devices
  devs = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
  found_preferred_device = false;
  for (int i=0;i<devs.size();i++) {
    audio_input_devices->addItem(devs.at(i).deviceName(), devs.at(i).deviceName());
    if (!found_preferred_device
        && devs.at(i).deviceName() == olive::CurrentConfig.preferred_audio_input) {
      audio_input_devices->setCurrentIndex(audio_input_devices->count()-1);
      found_preferred_device = true;
    }
  }

  audio_tab_layout->addWidget(audio_input_devices, row, 1);

  row++;

  // Audio -> Sample Rate

  audio_tab_layout->addWidget(new QLabel(tr("Sample Rate:")), row, 0);

  audio_sample_rate = new QComboBox();
  combobox_audio_sample_rates(audio_sample_rate);
  for (int i=0;i<audio_sample_rate->count();i++) {
    if (audio_sample_rate->itemData(i).toInt() == olive::CurrentConfig.audio_rate) {
      audio_sample_rate->setCurrentIndex(i);
      break;
    }
  }

  audio_tab_layout->addWidget(audio_sample_rate, row, 1);

  row++;

  // Audio -> Audio Recording
  audio_tab_layout->addWidget(new QLabel(tr("Audio Recording:"), this), row, 0);

  recordingComboBox = new QComboBox(general_tab);
  recordingComboBox->addItem(tr("Mono"));
  recordingComboBox->addItem(tr("Stereo"));
  audio_tab_layout->addWidget(recordingComboBox, row, 1);

  row++;

  tabWidget->addTab(audio_tab, tr("Audio"));

  // Shortcuts
  QWidget* shortcut_tab = new QWidget(this);

  QVBoxLayout* shortcut_layout = new QVBoxLayout(shortcut_tab);

  QLineEdit* key_search_line = new QLineEdit(shortcut_tab);
  key_search_line->setPlaceholderText(tr("Search for action or shortcut"));
  connect(key_search_line, SIGNAL(textChanged(const QString &)), this, SLOT(refine_shortcut_list(const QString &)));

  shortcut_layout->addWidget(key_search_line);

  keyboard_tree = new QTreeWidget(shortcut_tab);
  QTreeWidgetItem* tree_header = keyboard_tree->headerItem();
  tree_header->setText(0, tr("Action"));
  tree_header->setText(1, tr("Shortcut"));
  shortcut_layout->addWidget(keyboard_tree);

  QHBoxLayout* reset_shortcut_layout = new QHBoxLayout(shortcut_tab);

  QPushButton* import_shortcut_button = new QPushButton(tr("Import"), shortcut_tab);
  reset_shortcut_layout->addWidget(import_shortcut_button);
  connect(import_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(load_shortcut_file()));

  QPushButton* export_shortcut_button = new QPushButton(tr("Export"), shortcut_tab);
  reset_shortcut_layout->addWidget(export_shortcut_button);
  connect(export_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(save_shortcut_file()));

  reset_shortcut_layout->addStretch();

  QPushButton* reset_selected_shortcut_button = new QPushButton(tr("Reset Selected"), shortcut_tab);
  reset_shortcut_layout->addWidget(reset_selected_shortcut_button);
  connect(reset_selected_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(reset_default_shortcut()));

  QPushButton* reset_all_shortcut_button = new QPushButton(tr("Reset All"), shortcut_tab);
  reset_shortcut_layout->addWidget(reset_all_shortcut_button);
  connect(reset_all_shortcut_button, SIGNAL(clicked(bool)), this, SLOT(reset_all_shortcuts()));

  shortcut_layout->addLayout(reset_shortcut_layout);

  tabWidget->addTab(shortcut_tab, tr("Keyboard"));

  verticalLayout->addWidget(tabWidget);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
  buttonBox->setOrientation(Qt::Horizontal);
  buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

  verticalLayout->addWidget(buttonBox);

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(save()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}
