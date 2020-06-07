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

#include "preferencesgeneraltab.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include "common/autoscroll.h"
#include "dialog/sequence/sequence.h"
#include "project/item/sequence/sequence.h"

OLIVE_NAMESPACE_ENTER

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
  default_still_length_->SetFormat(tr("%1 seconds"));
  default_still_length_->SetValue(Config::Current()["DefaultStillLength"].value<rational>().toDouble());
  general_layout->addWidget(default_still_length_);

  layout->addStretch();
}

void PreferencesGeneralTab::Accept()
{
  Config::Current()["RectifiedWaveforms"] = rectified_waveforms_->isChecked();

  Config::Current()["Autoscroll"] = autoscroll_method_->currentData();

  Config::Current()["DefaultStillLength"] = QVariant::fromValue(rational::fromDouble(default_still_length_->GetValue()));
}

OLIVE_NAMESPACE_EXIT
