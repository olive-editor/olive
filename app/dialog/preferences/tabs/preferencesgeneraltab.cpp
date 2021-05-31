/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include "common/autoscroll.h"
#include "core.h"
#include "dialog/sequence/sequence.h"
#include "node/project/sequence/sequence.h"

namespace olive {

PreferencesGeneralTab::PreferencesGeneralTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  {
    QGroupBox* global_groupbox = new QGroupBox(tr("Locale"));
    QGridLayout* global_layout = new QGridLayout(global_groupbox);
    layout->addWidget(global_groupbox);

    int row = 0;

    // General -> Language
    global_layout->addWidget(new QLabel(tr("Language:")), row, 0);

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

    global_layout->addWidget(language_combobox_, row, 1);
  }

  {
    QGroupBox* timeline_groupbox = new QGroupBox(tr("Timeline"));
    QGridLayout* timeline_layout = new QGridLayout(timeline_groupbox);
    layout->addWidget(timeline_groupbox);

    int row = 0;

    QLabel* autoscroll_lbl = new QLabel(tr("Auto-Scroll Method:"));
    autoscroll_lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    timeline_layout->addWidget(autoscroll_lbl, row, 0);

    // ComboBox indices match enum indices
    autoscroll_method_ = new QComboBox();
    autoscroll_method_->addItem(tr("None"), AutoScroll::kNone);
    autoscroll_method_->addItem(tr("Page Scrolling"), AutoScroll::kPage);
    autoscroll_method_->addItem(tr("Smooth Scrolling"), AutoScroll::kSmooth);
    autoscroll_method_->setCurrentIndex(Config::Current()["Autoscroll"].toInt());
    timeline_layout->addWidget(autoscroll_method_, row, 1);

    row++;

    timeline_layout->addWidget(new QLabel(tr("Rectified Waveforms:")), row, 0);

    rectified_waveforms_ = new QCheckBox();
    rectified_waveforms_->setChecked(Config::Current()["RectifiedWaveforms"].toBool());
    timeline_layout->addWidget(rectified_waveforms_, row, 1);

    row++;

    timeline_layout->addWidget(new QLabel(tr("Default Still Image Length:")), row, 0);

    default_still_length_ = new RationalSlider();
    default_still_length_->SetMinimum(rational(100, 1000));
    default_still_length_->SetTimebase(rational(100, 1000));
    default_still_length_->SetFormat(tr("%1 seconds"));
    default_still_length_->SetValue(Config::Current()["DefaultStillLength"].value<rational>());
    timeline_layout->addWidget(default_still_length_);
  }

  {
    QGroupBox* autorecovery_groupbox = new QGroupBox(tr("Auto-Recovery"));
    QGridLayout* autorecovery_layout = new QGridLayout(autorecovery_groupbox);
    layout->addWidget(autorecovery_groupbox);

    int row = 0;

    autorecovery_layout->addWidget(new QLabel(tr("Enable Auto-Recovery:")), row, 0);

    autorecovery_enabled_ = new QCheckBox();
    autorecovery_enabled_->setChecked(Config::Current()[QStringLiteral("AutorecoveryEnabled")].toBool());
    autorecovery_layout->addWidget(autorecovery_enabled_, row, 1);

    row++;

    autorecovery_layout->addWidget(new QLabel(tr("Auto-Recovery Interval:")), row, 0);

    autorecovery_interval_ = new IntegerSlider();
    autorecovery_interval_->SetMinimum(1);
    autorecovery_interval_->SetMaximum(60);
    autorecovery_interval_->SetFormat(QT_TRANSLATE_N_NOOP("olive::SliderBase", "%n minute(s)"), true);
    autorecovery_interval_->SetValue(Config::Current()[QStringLiteral("AutorecoveryInterval")].toLongLong());
    autorecovery_layout->addWidget(autorecovery_interval_, row, 1);

    row++;

    autorecovery_layout->addWidget(new QLabel(tr("Maximum Versions Per Project:")), row, 0);

    autorecovery_maximum_ = new IntegerSlider();
    autorecovery_maximum_->SetMinimum(1);
    autorecovery_maximum_->SetMaximum(1000);
    autorecovery_maximum_->SetValue(Config::Current()[QStringLiteral("AutorecoveryMaximum")].toLongLong());
    autorecovery_layout->addWidget(autorecovery_maximum_, row, 1);

    row++;

    QPushButton* browse_autorecoveries = new QPushButton(tr("Browse Auto-Recoveries"));
    connect(browse_autorecoveries, &QPushButton::clicked, Core::instance(), &Core::BrowseAutoRecoveries);
    autorecovery_layout->addWidget(browse_autorecoveries, row, 1);
  }

  layout->addStretch();
}

void PreferencesGeneralTab::Accept(MultiUndoCommand *command)
{
  Q_UNUSED(command)

  Config::Current()[QStringLiteral("RectifiedWaveforms")] = rectified_waveforms_->isChecked();

  Config::Current()[QStringLiteral("Autoscroll")] = autoscroll_method_->currentData();

  Config::Current()[QStringLiteral("DefaultStillLength")] = QVariant::fromValue(default_still_length_->GetValue());

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

  Config::Current()[QStringLiteral("AutorecoveryEnabled")] = autorecovery_enabled_->isChecked();
  Config::Current()[QStringLiteral("AutorecoveryInterval")] = QVariant::fromValue(autorecovery_interval_->GetValue());
  Config::Current()[QStringLiteral("AutorecoveryMaximum")] = QVariant::fromValue(autorecovery_maximum_->GetValue());
  Core::instance()->SetAutorecoveryInterval(autorecovery_interval_->GetValue());
}

void PreferencesGeneralTab::AddLanguage(const QString &locale_name)
{
  language_combobox_->addItem(tr("%1 (%2)").arg(QLocale(locale_name).nativeLanguageName(),
                                                locale_name));;
  language_combobox_->setItemData(language_combobox_->count() - 1, locale_name);
}

}
