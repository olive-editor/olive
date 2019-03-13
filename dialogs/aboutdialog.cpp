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

#include "aboutdialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

#include "global/global.h"

AboutDialog::AboutDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowTitle("About Olive");

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(20);

  QLabel* label =
      new QLabel(QString("<html><head/><body>"
                         "<p><img src=\":/icons/olive-splash.png\"/></p>"
                         "<p><a href=\"https://www.olivevideoeditor.org/\">"
                         "<span style=\" text-decoration: underline; color:#007af4;\">"
                         "https://www.olivevideoeditor.org/"
                         "</span></a></p>"
                         "<p><b>%1</b></p>" // AppName (version identifier)
                         "<p>%2</p>" // First statement
                         "<p>%3</p>" // Second statement
                         "</body></html>").arg(olive::AppName,
                                               tr("Olive is a non-linear video editor. This software is free and "
                                                  "protected by the GNU GPL."),
                                               tr("Olive Team is obliged to inform users that Olive source code is "
                                                  "available for download from its website.")), this);
  label->setAlignment(Qt::AlignCenter);
  label->setWordWrap(true);
  layout->addWidget(label);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons);

  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
}
