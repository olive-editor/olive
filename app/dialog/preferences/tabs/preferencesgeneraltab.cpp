#include "preferencesgeneraltab.h"

#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

PreferencesGeneralTab::PreferencesGeneralTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QGridLayout* general_layout = new QGridLayout();
  layout->addLayout(general_layout);

  int row = 0;

  // General -> Language
  general_layout->addWidget(new QLabel(tr("Language:")), row, 0);

  language_combobox = new QComboBox();

  // add default language (en-US)
  language_combobox->addItem(QLocale::languageToString(QLocale("en-US").language()));

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

  general_layout->addWidget(language_combobox, row, 1, 1, 4);

  row++;

  // General -> Thumbnail and Waveform Resolution
  general_layout->addWidget(new QLabel(tr("Thumbnail Resolution:"), this), row, 0);

  thumbnail_res_spinbox = new QSpinBox(this);
  thumbnail_res_spinbox->setMinimum(0);
  thumbnail_res_spinbox->setMaximum(INT_MAX);
  //thumbnail_res_spinbox->setValue(olive::config.thumbnail_resolution);
  general_layout->addWidget(thumbnail_res_spinbox, row, 1);

  general_layout->addWidget(new QLabel(tr("Waveform Resolution:"), this), row, 2);

  waveform_res_spinbox = new QSpinBox(this);
  waveform_res_spinbox->setMinimum(0);
  waveform_res_spinbox->setMaximum(INT_MAX);
  //waveform_res_spinbox->setValue(olive::config.waveform_resolution);
  general_layout->addWidget(waveform_res_spinbox, row, 3);

  QPushButton* delete_preview_btn = new QPushButton(tr("Delete Previews"));
  general_layout->addWidget(delete_preview_btn, row, 4);
  //connect(delete_preview_btn, SIGNAL(clicked(bool)), this, SLOT(delete_all_previews()));

  row++;

  QHBoxLayout* misc_general = new QHBoxLayout();

  // General -> Default Sequence Settings
  QPushButton* default_sequence_settings = new QPushButton(tr("Default Sequence Settings"));
  connect(default_sequence_settings, SIGNAL(clicked(bool)), this, SLOT(edit_default_sequence_settings()));
  misc_general->addWidget(default_sequence_settings);

  general_layout->addLayout(misc_general, row, 0, 1, 5);

  layout->addStretch();
}

void PreferencesGeneralTab::Accept()
{

}

void PreferencesGeneralTab::edit_default_sequence_settings()
{
  /*NewSequenceDialog nsd(this, nullptr, &default_sequence);
  nsd.SetNameEditable(false);
  nsd.exec();*/
}
