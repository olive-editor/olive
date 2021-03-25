/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include "core.h"
#include "dialog/sequence/sequence.h"
#include "project/item/sequence/sequence.h"

namespace olive {

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
  QDir language_dir(QStringLiteral(":/ts"));
  QStringList languages = language_dir.entryList();
  foreach (const QString& l, languages) {
    AddLanguage(l);
  }

  QString current_language = Config::Current()[QStringLiteral("Language")].toString();
  if (current_language.isEmpty()) {
    // No configured language, use system language
    current_language = QLocale::system().name();

    // If we don't have a language for this, default to en_US
    if (!languages.contains(current_language)) {
      current_language = QStringLiteral("en_US");
    }
  }
  language_combobox_->setCurrentIndex(languages.indexOf(current_language));

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
  default_still_length_->SetFormat(tr("%1 second(s)"));
  default_still_length_->SetValue(Config::Current()["DefaultStillLength"].value<rational>().toDouble());
  general_layout->addWidget(default_still_length_);

  layout->addStretch();
}

void PreferencesGeneralTab::Accept(MultiUndoCommand *command)
{
  Q_UNUSED(command)

  Config::Current()[QStringLiteral("RectifiedWaveforms")] = rectified_waveforms_->isChecked();

  Config::Current()[QStringLiteral("Autoscroll")] = autoscroll_method_->currentData();

  Config::Current()[QStringLiteral("DefaultStillLength")] = QVariant::fromValue(rational::fromDouble(default_still_length_->GetValue()));

  QString set_language = language_combobox_->currentData().toString();
  if (QLocale::system().name() == set_language) {
    // Language is set to the system, assume this is effectively "auto"
    set_language = QString();
  }

  // If the language has changed, set it now
  if (Config::Current()[QStringLiteral("Language")].toString() != set_language) {
    Config::Current()[QStringLiteral("Language")] = set_language;
    Core::instance()->SetLanguage(set_language.isEmpty() ? QLocale::system().name() : set_language);
  }
}

void PreferencesGeneralTab::AddLanguage(const QString &locale_name)
{
  language_combobox_->addItem(tr("%1 (%2)").arg(QLocale(locale_name).nativeLanguageName(),
                                                locale_name));;
  language_combobox_->setItemData(language_combobox_->count() - 1, locale_name);
}

}
