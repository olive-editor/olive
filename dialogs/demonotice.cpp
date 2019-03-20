/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "demonotice.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

DemoNotice::DemoNotice(QWidget *parent) :
  QDialog(parent)
{
    setWindowTitle(tr("Welcome to Olive!"));

  QVBoxLayout* vlayout = new QVBoxLayout(this);

  QHBoxLayout* layout = new QHBoxLayout();
  layout->setMargin(10);
  layout->setSpacing(20);

  QLabel* icon = new QLabel("<html><head/><body>"
                "<p><img src=\":/icons/olive-splash.png\"/></p>"
                "</body></html>", this);
  layout->addWidget(icon);

  QLabel* text = new QLabel("<html><head/><body><p>"
                "<span style=\" font-size:14pt;\">"
                + tr("Welcome to Olive!")
                + "</span></p><p>"
                + tr("Olive is a free open-source video editor released under the GNU GPL. If you have paid for this software, you have been scammed.")
                + "</p><p>"
                + tr("This software is currently in ALPHA which means it is unstable and very likely to crash, have bugs, and have missing features. We offer no warranty so use at your own risk. Please report any bugs or feature requests at %1").arg("<a href=\"https://olivevideoeditor.org/\"><span style=\" text-decoration: underline; color:#007af4;\">www.olivevideoeditor.org</span></a>")
                   + "</p><p>"
                   + tr("Thank you for trying Olive and we hope you enjoy it!")
                   + "</p></body></html>", this);
  text->setWordWrap(true);
  layout->addWidget(text);

  vlayout->addLayout(layout);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
  buttons->setCenterButtons(true);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  vlayout->addWidget(buttons);
}
