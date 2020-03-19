#include "preferencesgeneraltab.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "common/autoscroll.h"
#include "dialog/sequence/sequence.h"
#include "project/item/sequence/sequence.h"

PreferencesGeneralTab::PreferencesGeneralTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  QGridLayout* general_layout = new QGridLayout();
  layout->addLayout(general_layout);

  int row = 0;

  // General -> Language
  general_layout->addWidget(new QLabel(tr("Language:")), row, 0);

  language_combobox_ = new QComboBox();

  // Add default language (en-US)
  language_combobox_->addItem(QLocale("en_US").nativeLanguageName());

  // Set sequence to pick up default parameters from the config
  default_sequence_.set_default_parameters();

  /*
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

          if (olive::config.language_file == locale_relative_path) {
            language_combobox->setCurrentIndex(language_combobox->count() - 1);
          }
        }
      }
    }
    */

  general_layout->addWidget(language_combobox_, row, 1);

  row++;

  general_layout->addWidget(new QLabel(tr("Auto-Scroll Method:")), row, 0);

  // ComboBox indices match enum indices
  autoscroll_method_ = new QComboBox();
  autoscroll_method_->addItem(tr("None"), AutoScroll::kNone);
  autoscroll_method_->addItem(tr("Page Scrolling"), AutoScroll::kPage);
  autoscroll_method_->addItem(tr("Smooth Scrolling"), AutoScroll::kSmooth);
  autoscroll_method_->setCurrentIndex(Config::Current()["Autoscroll"].toInt());
  general_layout->addWidget(autoscroll_method_, row, 1);

  row++;

  general_layout->addWidget(new QLabel(tr("Rectified Waveforms:")), row, 0);

  rectified_waveforms_ = new QCheckBox();
  rectified_waveforms_->setChecked(Config::Current()["RectifiedWaveforms"].toBool());
  general_layout->addWidget(rectified_waveforms_, row, 1);

  row++;

  general_layout->addWidget(new QLabel(tr("Default Still Image Length:")), row, 0);

  default_still_length_ = new FloatSlider();
  default_still_length_->SetMinimum(0.1);
  default_still_length_->SetValue(Config::Current()["DefaultStillLength"].value<rational>().toDouble());
  general_layout->addWidget(default_still_length_);

  row++;

  general_layout->addWidget(new QLabel(tr("Default Sequence Settings:")), row, 0);

  // General -> Default Sequence Settings
  QPushButton* default_sequence_settings = new QPushButton(tr("Edit"));
  connect(default_sequence_settings, SIGNAL(clicked(bool)), this, SLOT(edit_default_sequence_settings()));
  general_layout->addWidget(default_sequence_settings, row, 1);

  layout->addStretch();
}

void PreferencesGeneralTab::Accept()
{
  Config::Current()["DefaultSequenceWidth"] = default_sequence_.video_params().width();
  Config::Current()["DefaultSequenceHeight"] = default_sequence_.video_params().height();;
  Config::Current()["DefaultSequenceFrameRate"] = QVariant::fromValue(default_sequence_.video_params().time_base());
  Config::Current()["DefaultSequenceAudioFrequency"] = default_sequence_.audio_params().sample_rate();
  Config::Current()["DefaultSequenceAudioLayout"] = QVariant::fromValue(default_sequence_.audio_params().channel_layout());

  Config::Current()["RectifiedWaveforms"] = rectified_waveforms_->isChecked();

  Config::Current()["Autoscroll"] = autoscroll_method_->currentData();

  Config::Current()["DefaultStillLength"] = QVariant::fromValue(rational::fromDouble(default_still_length_->GetValue()));
}

void PreferencesGeneralTab::edit_default_sequence_settings()
{
  SequenceDialog sd(&default_sequence_, SequenceDialog::kExisting, this);
  sd.SetUndoable(false);
  sd.SetNameIsEditable(false);
  sd.exec();
}
