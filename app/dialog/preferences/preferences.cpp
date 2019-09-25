/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "preferences.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "config/config.h"
#include "tabs/preferencesgeneraltab.h"
#include "tabs/preferencesbehaviortab.h"
#include "tabs/preferencesappearancetab.h"
#include "tabs/preferencesplaybacktab.h"
#include "tabs/preferencesaudiotab.h"
#include "tabs/preferencescolormanagementtab.h"
#include "tabs/preferenceskeyboardtab.h"

PreferencesDialog::PreferencesDialog(QWidget *parent, QMenuBar* main_menu_bar) :
  QDialog(parent)
{
  setWindowTitle(tr("Preferences"));

  QVBoxLayout* verticalLayout = new QVBoxLayout(this);
  tab_widget_ = new QTabWidget(this);

  AddTab(new PreferencesGeneralTab(), tr("General"));
  AddTab(new PreferencesAppearanceTab(), tr("Appearance"));
  AddTab(new PreferencesBehaviorTab(), tr("Behavior"));
  AddTab(new PreferencesColorManagementTab(), tr("Color Management"));
  AddTab(new PreferencesPlaybackTab(), tr("Playback"));
  AddTab(new PreferencesAudioTab(), tr("Audio"));
  AddTab(new PreferencesKeyboardTab(main_menu_bar), tr("Keyboard"));

  verticalLayout->addWidget(tab_widget_);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
  buttonBox->setOrientation(Qt::Horizontal);
  buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

  verticalLayout->addWidget(buttonBox);

  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

  // set up default sequence
  /*default_sequence.set_name(tr("Default Sequence"));
  default_sequence.set_width(olive::config.default_sequence_width);
  default_sequence.set_height(olive::config.default_sequence_height);
  default_sequence.set_frame_rate(olive::config.default_sequence_framerate);
  default_sequence.set_audio_frequency(olive::config.default_sequence_audio_frequency);
  default_sequence.set_audio_layout(olive::config.default_sequence_audio_channel_layout);*/
}

void PreferencesDialog::AddBoolPair(QCheckBox *ui, bool *value, bool restart_required)
{
  bool_ui.append(ui);
  bool_value.append(value);
  bool_restart_required.append(restart_required);

  ui->setChecked(*value);
}

void PreferencesDialog::accept()
{
  foreach (PreferencesTab* tab, tabs_) {
    tab->Accept();
  }

  QDialog::accept();

  /*
  bool restart_after_saving = false;
  bool reinit_audio = false;
  bool reload_language = false;
  bool reload_effects = false;
  bool reset_ocio_shaders = false;
  bool reset_render_threads = false;

  // Validate whether the specified CSS file exists
  if (!custom_css_fn->text().isEmpty() && !QFileInfo::exists(custom_css_fn->text())) {
    QMessageBox::critical(
          this,
          tr("Invalid CSS File"),
          tr("CSS file '%1' does not exist.").arg(custom_css_fn->text())
          );
    return;
  }

  // Validate whether the chosen OCIO configuration file
  if (enable_color_management->isChecked()) {

    // Check whether the file exists
    if (!QFileInfo::exists(ocio_config_file->text())) {

      QString msg_title = tr("Invalid OpenColorIO Configuration File");
      QString msg_body;

      if (ocio_config_file->text().isEmpty()) {
        msg_body = tr("You must specify an OpenColorIO configuration file if color management is enabled.");
      } else {
        msg_body = tr("OpenColorIO configuration file '%1' does not exist.").arg(ocio_config_file->text());
      }

      QMessageBox::critical(
            this,
            msg_title,
            msg_body
            );
      return;

    } else if (olive::config.ocio_config_path != ocio_config_file->text()) {

      // Check whether OCIO can load it
      OCIO::ConstConfigRcPtr file_config = TestOCIOConfig(ocio_config_file->text().toUtf8());

      if (!file_config) {
        return;
      }

    }
  }

  // Validate whether one of the bool options requires a restart
  bool bool_requires_restart = false;
  for (int i=0;i<bool_restart_required.size();i++) {
    if (bool_restart_required.at(i)
        && bool_ui.at(i)->isChecked() != *bool_value.at(i)) {
      bool_requires_restart = true;
      break;
    }
  }

  // Check if any settings will require a restart of Olive (including the bool options determined above)
  if (bool_requires_restart
      || olive::config.thumbnail_resolution != thumbnail_res_spinbox->value()
      || olive::config.waveform_resolution != waveform_res_spinbox->value()
      || olive::config.css_path != custom_css_fn->text()
      || olive::config.style != static_cast<olive::styling::Style>(ui_style->currentData().toInt())) {

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


  // Everything checks out, start saving settings from the UI to the backend
  olive::config.css_path = custom_css_fn->text();
  olive::config.recording_mode = recordingComboBox->currentIndex() + 1;
  olive::config.img_seq_formats = imgSeqFormatEdit->text();
  olive::config.upcoming_queue_size = upcoming_queue_spinbox->value();
  olive::config.upcoming_queue_type = upcoming_queue_type->currentIndex();
  olive::config.previous_queue_size = previous_queue_spinbox->value();
  olive::config.previous_queue_type = previous_queue_type->currentIndex();

  // Audio settings may require the audio device to be re-initiated.
  if (kUsingAudioOutput != audio_output_devices->currentData().toString()
      || olive::config.preferred_audio_input != audio_input_devices->currentData().toString()
      || olive::config.audio_rate != audio_sample_rate->currentData().toInt()) {
    reinit_audio = true;
  }
  kUsingAudioOutput = audio_output_devices->currentData().toString();
  olive::config.preferred_audio_input = audio_input_devices->currentData().toString();
  olive::config.audio_rate = audio_sample_rate->currentData().toInt();

  olive::config.effect_textbox_lines = effect_textbox_lines_field->value();

  // see if the language file should be reloaded (not necessary if the app is restarting anyway)
  if (!restart_after_saving
      && olive::config.language_file != language_combobox->currentData().toString()) {
    reload_language = true;
  }
  olive::config.language_file = language_combobox->currentData().toString();

  // Check whether OCIO settings will require a reset of the render threads
  if (olive::config.playback_bit_depth != playback_bit_depth->currentIndex()
      || olive::config.export_bit_depth != export_bit_depth->currentIndex()) {
    reset_render_threads = true;
  }
  if (olive::config.ocio_config_path != ocio_config_file->text()
      || olive::config.ocio_display != ocio_display->currentText()
      || olive::config.ocio_view != ocio_view->currentText()
      || olive::config.ocio_look != ocio_look->currentData().toString()) {
    reset_ocio_shaders = true;
  }
  if (olive::config.ocio_config_path != ocio_config_file->text()) {
    OCIO::SetCurrentConfig(OCIO::Config::CreateFromFile(ocio_config_file->text().toUtf8()));
    olive::config.ocio_config_path = ocio_config_file->text();
  }
  olive::config.enable_color_management = enable_color_management->isChecked();
  olive::config.playback_bit_depth = playback_bit_depth->currentIndex();
  olive::config.export_bit_depth = export_bit_depth->currentIndex();
  olive::config.ocio_display = ocio_display->currentText();
  olive::config.ocio_default_input_colorspace = ocio_default_input->currentText();
  olive::config.ocio_view = ocio_view->currentText();

  // We use data here instead of text because there's a "(None)" option with an empty string
  olive::config.ocio_look = ocio_look->currentData().toString();


  // Set default sequence options
  olive::config.default_sequence_width = default_sequence.width();
  olive::config.default_sequence_height = default_sequence.height();
  olive::config.default_sequence_framerate = default_sequence.frame_rate();
  olive::config.default_sequence_audio_frequency = default_sequence.audio_frequency();
  olive::config.default_sequence_audio_channel_layout = default_sequence.audio_layout();

  // Set all bool options
  for (int i=0;i<bool_ui.size();i++) {
    *bool_value[i] = bool_ui.at(i)->isChecked();
  }

  // Set new style
  olive::config.style = static_cast<olive::styling::Style>(ui_style->currentData().toInt());

  // Check if the thumbnail or waveform icon fields have changed, we may need to recreate the previews if so
  if (olive::config.thumbnail_resolution != thumbnail_res_spinbox->value()
      || olive::config.waveform_resolution != waveform_res_spinbox->value()) {
    // we're changing the size of thumbnails and waveforms, so let's delete them and regenerate them next start

    // delete nothing
    PreviewDeleteTypes delete_type = DELETE_NONE;

    if (olive::config.thumbnail_resolution != thumbnail_res_spinbox->value()) {
      // delete existing thumbnails
      olive::config.thumbnail_resolution = thumbnail_res_spinbox->value();

      // delete only thumbnails
      delete_type = DELETE_THUMBNAILS;
    }

    if (olive::config.waveform_resolution != waveform_res_spinbox->value()) {
      // delete existing waveforms
      olive::config.waveform_resolution = waveform_res_spinbox->value();

      // if we're already deleting thumbnails
      if (delete_type == DELETE_THUMBNAILS) {
        // delete all
        delete_type = DELETE_BOTH;
      } else {
        // just delete waveforms
        delete_type = DELETE_WAVEFORMS;
      }
    }

    delete_previews(delete_type);
  }



  QDialog::accept();

  if (restart_after_saving) {

    // since we already ran can_close_project(), bypass checking again by running set_modified(false)
    olive::Global->set_modified(false);

    olive::MainWindow->close();

    QProcess::startDetached(QApplication::applicationFilePath(), { olive::ActiveProjectFilename });
  } else {

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

    if (reset_render_threads) {
      if (panel_footage_viewer->seq != nullptr) {
        panel_footage_viewer->seq->Close();
      }
      panel_footage_viewer->viewer_widget()->get_renderer()->delete_ctx();
      if (panel_sequence_viewer->seq != nullptr) {
        panel_sequence_viewer->seq->Close();
      }
      panel_sequence_viewer->viewer_widget()->get_renderer()->delete_ctx();
    } else if (reset_ocio_shaders) {
      panel_footage_viewer->viewer_widget()->get_renderer()->destroy_ocio();
      panel_sequence_viewer->viewer_widget()->get_renderer()->destroy_ocio();
    }

  }
  */
}

void PreferencesDialog::AddTab(PreferencesTab *tab, const QString &title)
{
  tab_widget_->addTab(tab, title);

  tabs_.append(tab);
}
