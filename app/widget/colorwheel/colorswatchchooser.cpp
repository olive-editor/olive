/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "colorswatchchooser.h"

#include <QGridLayout>

#include "common/filefunctions.h"
#include "widget/menu/menu.h"

namespace olive {

const int kDefaultColorCount = 16;
const Color kDefaultColors[kDefaultColorCount] = {
  Color(1.0, 1.0, 1.0),
  Color(1.0, 1.0, 0.0),
  Color(1.0, 0.5, 0.0),
  Color(1.0, 0.0, 0.0),
  Color(1.0, 0.0, 1.0),
  Color(0.5, 0.0, 1.0),
  Color(0.0, 0.0, 1.0),
  Color(0.0, 0.5, 1.0),
  Color(0.0, 1.0, 0.0),
  Color(0.0, 0.5, 0.0),
  Color(0.5, 0.25, 0.0),
  Color(0.75, 0.5, 0.25),
  Color(0.75, 0.75, 0.75),
  Color(0.5, 0.5, 0.5),
  Color(0.25, 0.25, 0.25),
  Color(0.0, 0.0, 0.0)
};

ColorSwatchChooser::ColorSwatchChooser(ColorManager *manager, QWidget *parent) :
  QWidget(parent)
{
  auto layout = new QGridLayout(this);

  for (int x=0; x<kColCount; x++) {
    for (int y=0; y<kRowCount; y++) {
      // Create button
      auto b = new ColorButton(manager, false);
      b->setFixedWidth(b->sizeHint().height()/2*3);
      b->setContextMenuPolicy(Qt::CustomContextMenu);
      layout->addWidget(b, y, x);

      // Save button in buttons array
      int btn_index = x + kColCount*y;
      buttons_[btn_index] = b;

      // Set default color
      SetDefaultColor(btn_index);

      // Connect clicks
      connect(b, &ColorButton::clicked, this, &ColorSwatchChooser::HandleButtonClick);
      connect(b, &ColorButton::customContextMenuRequested, this, &ColorSwatchChooser::HandleContextMenu);
    }
  }

  LoadSwatches();
}

void ColorSwatchChooser::SetDefaultColor(int index)
{
  if (index < kDefaultColorCount) {
    buttons_[index]->SetColor(kDefaultColors[index]);
  } else {
    buttons_[index]->SetColor(Color(1.0, 1.0, 1.0));
  }
}

void ColorSwatchChooser::HandleButtonClick()
{
  auto b = static_cast<ColorButton*>(sender());

  emit ColorClicked(b->GetColor());
  SetCurrentColor(b->GetColor());
}

void ColorSwatchChooser::HandleContextMenu()
{
  Menu m(this);

  auto save_action = m.addAction(tr("Save Color Here"));
  connect(save_action, &QAction::triggered, this, &ColorSwatchChooser::SaveCurrentColor);

  m.addSeparator();

  auto reset_action = m.addAction(tr("Reset To Default"));
  connect(reset_action, &QAction::triggered, this, &ColorSwatchChooser::ResetMenuButton);

  menu_btn_ = static_cast<ColorButton*>(sender());

  m.exec(QCursor::pos());
}

void ColorSwatchChooser::SaveCurrentColor()
{
  menu_btn_->SetColor(current_);

  SaveSwatches();
}

void ColorSwatchChooser::ResetMenuButton()
{
  for (int i=0; i<kBtnCount; i++) {
    if (buttons_[i] == menu_btn_) {
      SetDefaultColor(i);
      break;
    }
  }
}

QString ColorSwatchChooser::GetSwatchFilename()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("swatch"));
}

void ColorSwatchChooser::LoadSwatches()
{
  QFile f(GetSwatchFilename());
  if (f.open(QFile::ReadOnly)) {
    QDataStream d(&f);

    uint version;
    d >> version;

    if (version == 1) {
      int index = 0;
      while (index < kBtnCount && !d.atEnd()) {
        Color::DataType r;
        QString s;
        ManagedColor c;
        ColorTransform t;
        bool is_display;

        c.set_alpha(1.0);

        d >> r;
        c.set_red(r);

        d >> r;
        c.set_green(r);

        d >> r;
        c.set_blue(r);

        d >> s;
        c.set_color_input(s);

        d >> is_display;
        if (is_display) {
          QString display, view, look;
          d >> display;
          d >> view;
          d >> look;
          c.set_color_output(ColorTransform(display, view, look));
        } else {
          d >> s;
          c.set_color_output(ColorTransform(s));
        }

        buttons_[index]->SetColor(c);

        index++;
      }
    }

    f.close();
  }
}

void ColorSwatchChooser::SaveSwatches()
{
  QString fn = GetSwatchFilename();
  QFile f(fn);

  if (f.open(QFile::WriteOnly)) {
    QDataStream d(&f);

    const uint version = 1;
    d << version;

    for (int i=0; i<kBtnCount; i++) {
      const ManagedColor &c = buttons_[i]->GetColor();
      d << c.red();
      d << c.green();
      d << c.blue();
      d << c.color_input();
      d << c.color_output().is_display();

      if (c.color_output().is_display()) {
        d << c.color_output().display();
        d << c.color_output().view();
        d << c.color_output().look();
      } else {
        d << c.color_output().output();
      }
    }

    f.close();
  } else {
    qCritical() << "Failed to open swatch file" << fn << "for writing";
  }
}

}
