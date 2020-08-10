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

#include <QColorDialog>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>

#include "node/node.h"
#include "widget/colorbutton/colorbutton.h"

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

  style_combobox_ = new QComboBox();

  {
    const QMap<QString, QString>& themes = StyleManager::available_themes();
    QMap<QString, QString>::const_iterator i;
    for (i=themes.cbegin(); i!=themes.cend(); i++) {
      style_combobox_->addItem(i.value(), i.key());

      if (StyleManager::GetStyle() == i.key()) {
        style_combobox_->setCurrentIndex(style_combobox_->count()-1);
      }
    }
  }

  appearance_layout->addWidget(style_combobox_, row, 1);

  row++;

  {
    QGroupBox* color_group = new QGroupBox();
    color_group->setTitle(tr("Node Color Scheme"));

    QGridLayout* color_layout = new QGridLayout(color_group);

    for (int i=0; i<Node::kCategoryCount; i++) {
      QString cat_name = Node::GetCategoryName(static_cast<Node::CategoryID>(i));
      color_layout->addWidget(new QLabel(cat_name), i, 0);

      Color c = Config::Current()[QStringLiteral("NodeCatColor%1").arg(i)].value<Color>();
      colors_.append(c.toQColor());

      QPushButton* color_btn = new QPushButton();
      connect(color_btn, &QPushButton::clicked, this, &PreferencesAppearanceTab::ColorButtonClicked);
      color_layout->addWidget(color_btn, i, 1);
      color_btns_.append(color_btn);

      UpdateButtonColor(i);
    }

    appearance_layout->addWidget(color_group, row, 0, 1, 2);
  }

  layout->addStretch();
}

void PreferencesAppearanceTab::Accept()
{
  QString style_path = style_combobox_->currentData().toString();

  if (style_path != StyleManager::GetStyle()) {
    StyleManager::SetStyle(style_path);
    Config::Current()["Style"] = style_path;
  }

  for (int i=0;i<colors_.size();i++) {
    Config::Current()[QStringLiteral("NodeCatColor%1").arg(i)] = QVariant::fromValue(Color(colors_.at(i)));
  }
}

void PreferencesAppearanceTab::UpdateButtonColor(int index)
{
  color_btns_.at(index)->setStyleSheet(QStringLiteral("background: %1;")
                                       .arg(colors_.at(index).name()));
}

void PreferencesAppearanceTab::ColorButtonClicked()
{
  int index = color_btns_.indexOf(static_cast<QPushButton*>(sender()));

  QColor new_color = QColorDialog::getColor(colors_.at(index), this);

  if (new_color.isValid()) {
    colors_.replace(index, new_color);

    UpdateButtonColor(index);
  }
}

OLIVE_NAMESPACE_EXIT
