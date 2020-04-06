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

#include "preferencesappearancetab.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

OLIVE_NAMESPACE_ENTER

PreferencesAppearanceTab::PreferencesAppearanceTab()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(0);

  QGridLayout* appearance_layout = new QGridLayout();
  layout->addLayout(appearance_layout);

  int row = 0;

  // Appearance -> Theme
  appearance_layout->addWidget(new QLabel(tr("Theme")), row, 0);

  style_ = new QComboBox();

  style_list_ = StyleManager::ListInternal();

  foreach (StyleDescriptor s, style_list_) {
    style_->addItem(s.name(), s.path());

    if (s.path() == Config::Current()["Style"]) {
      style_->setCurrentIndex(style_->count()-1);
    }
  }

  appearance_layout->addWidget(style_, row, 1, 1, 2);

  row++;

  layout->addStretch();
}

void PreferencesAppearanceTab::Accept()
{
  QString style_path = style_->currentData().toString();
  StyleManager::SetStyle(style_path);
  Config::Current()["Style"] = style_path;

  if (style_->currentIndex() < style_list_.size()) {
    // This is an internal style, set accordingly

  } else {
    StyleManager::SetStyle(style_->currentData().toString());
  }
}

OLIVE_NAMESPACE_EXIT
