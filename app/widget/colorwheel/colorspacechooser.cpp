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

#include "colorspacechooser.h"

#include <QGridLayout>
#include <QLabel>

OLIVE_NAMESPACE_ENTER

ColorSpaceChooser::ColorSpaceChooser(ColorManager* color_manager, bool enable_input_field, bool enable_display_fields, QWidget *parent):
  QGroupBox(parent),
  color_manager_(color_manager)
{
  QGridLayout* layout = new QGridLayout(this);

  setWindowTitle(tr("Color Management"));

  int row = 0;

  if (enable_input_field) {

    QString field_text;

    if (enable_display_fields) {
      // If the display fields are visible, identify this as the input
      field_text = tr("Input:");
    } else {
      // Otherwise, this widget will essentially just serve as a list of standard color spaces
      field_text = tr("Color Space:");
    }

    layout->addWidget(new QLabel(field_text), row, 0);

    input_combobox_ = new QComboBox();
    layout->addWidget(input_combobox_, row, 1);

    QStringList input_spaces = color_manager->ListAvailableInputColorspaces();

    foreach (const QString& s, input_spaces) {
      input_combobox_->addItem(s);
    }

    if (!color_manager_->GetDefaultInputColorSpace().isEmpty()) {
      input_combobox_->setCurrentText(color_manager_->GetDefaultInputColorSpace());
    }

    connect(input_combobox_, &QComboBox::currentTextChanged, this, &ColorSpaceChooser::ComboBoxChanged);

    row++;
  } else {
    input_combobox_ = nullptr;
  }

  if (enable_display_fields) {
    {
      layout->addWidget(new QLabel(tr("Display:")), row, 0);

      display_combobox_ = new QComboBox();
      layout->addWidget(display_combobox_, row, 1);

      QStringList display_spaces = color_manager->ListAvailableDisplays();

      foreach (const QString& s, display_spaces) {
        display_combobox_->addItem(s);
      }

      display_combobox_->setCurrentText(color_manager_->GetDefaultDisplay());

      connect(display_combobox_, &QComboBox::currentTextChanged, this, &ColorSpaceChooser::ComboBoxChanged);
      connect(display_combobox_, &QComboBox::currentTextChanged, this, &ColorSpaceChooser::UpdateViews);
    }

    row++;

    {
      layout->addWidget(new QLabel(tr("View:")), row, 0);

      view_combobox_ = new QComboBox();
      layout->addWidget(view_combobox_, row, 1);

      UpdateViews(display_combobox_->currentText());

      connect(view_combobox_, &QComboBox::currentTextChanged, this, &ColorSpaceChooser::ComboBoxChanged);
    }

    row++;

    {
      layout->addWidget(new QLabel(tr("Look:")), row, 0);

      look_combobox_ = new QComboBox();
      layout->addWidget(look_combobox_, row, 1);

      QStringList looks = color_manager->ListAvailableLooks();

      look_combobox_->addItem(tr("(None)"), QString());

      foreach (const QString& s, looks) {
        look_combobox_->addItem(s, s);
      }

      connect(look_combobox_, &QComboBox::currentTextChanged, this, &ColorSpaceChooser::ComboBoxChanged);
    }
  }
}

QString ColorSpaceChooser::input() const
{
  if (input_combobox_) {
    return input_combobox_->currentText();
  } else {
    return QString();
  }
}

QString ColorSpaceChooser::display() const
{
  if (display_combobox_) {
    return display_combobox_->currentText();
  } else {
    return QString();
  }
}

QString ColorSpaceChooser::view() const
{
  if (view_combobox_) {
    return view_combobox_->currentText();
  } else {
    return QString();
  }
}

QString ColorSpaceChooser::look() const
{
  if (look_combobox_) {
    return look_combobox_->currentData().toString();
  } else {
    return QString();
  }
}

void ColorSpaceChooser::set_input(const QString &s)
{
  if (s.isEmpty()) {
    input_combobox_->setCurrentText(color_manager_->GetDefaultInputColorSpace());
  } else {
    input_combobox_->setCurrentText(s);
  }
}

void ColorSpaceChooser::set_display(const QString &s)
{
  if (s.isEmpty()) {
    display_combobox_->setCurrentText(color_manager_->GetDefaultDisplay());
  } else {
    display_combobox_->setCurrentText(s);
  }

  UpdateViews(display_combobox_->currentText());
}

void ColorSpaceChooser::set_view(const QString &s)
{
  if (s.isEmpty()) {
    view_combobox_->setCurrentText(color_manager_->GetDefaultView(display_combobox_->currentText()));
  } else {
    view_combobox_->setCurrentText(s);
  }
}

void ColorSpaceChooser::set_look(const QString &s)
{
  if (s.isEmpty()) {
    look_combobox_->setCurrentIndex(0);
  } else {
    look_combobox_->setCurrentText(s);
  }
}

void ColorSpaceChooser::UpdateViews(const QString& display)
{
  QString v = view_combobox_->currentText();

  view_combobox_->clear();

  QStringList views = color_manager_->ListAvailableViews(display);

  foreach (const QString& s, views) {
    view_combobox_->addItem(s);
  }

  if (views.contains(v)) {
    // If we have the view we had before, set it again
    view_combobox_->setCurrentText(v);
  } else {
    // Otherwise reset to default view for this display
    view_combobox_->setCurrentText(color_manager_->GetDefaultView(display));
  }
}

void ColorSpaceChooser::ComboBoxChanged()
{
  if (input_combobox_) {
    emit InputColorSpaceChanged(input());
  }

  if (display_combobox_) {
    emit DisplayColorSpaceChanged(display(), view(), look());
  }

  if (input_combobox_ && display_combobox_) {
    emit ColorSpaceChanged(input(), display(), view(), look());
  }
}

OLIVE_NAMESPACE_EXIT
