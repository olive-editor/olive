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

#include "about.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include "common/qtutils.h"
#include "patreon.h"
#include "scrollinglabel.h"

namespace olive {

AboutDialog::AboutDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowTitle(tr("About %1").arg(QApplication::applicationName()));

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel());

  QLabel* icon = new QLabel(QStringLiteral("<html><img src=':/graphics/olive-splash.png'></html>"));
  icon->setAlignment(Qt::AlignCenter);
  layout->addWidget(icon);

  // Construct About text
  QLabel* label =
      new QLabel(QStringLiteral("<html><head/><body>"
                                "<p><b>%1</b> %2</p>" // AppName (version identifier)
                                "<p><a href=\"https://www.olivevideoeditor.org/\">"
                                "https://www.olivevideoeditor.org/"
                                "</a></p>"
                                "<p>%3</p>" // First statement
                                "</body></html>").arg(QApplication::applicationName(),
                                                      QApplication::applicationVersion(),
                                                      tr("Olive is a free open source non-linear video editor. "
                                                         "This software is licensed under the GNU GPL Version 3.")));

  // Set text formatting
  label->setAlignment(Qt::AlignCenter);
  label->setWordWrap(true);
  label->setOpenExternalLinks(true);
  label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  layout->addWidget(label);

  // Patrons where necessary
  if (!patrons.isEmpty()) {
    layout->addWidget(new QLabel());

    QLabel* support_lbl = new QLabel(tr("<html>Olive wouldn't be possible without the support of gracious "
                                        "donations from <a href='https://www.patreon.com/olivevideoeditor'>Patreon</a></html>:"));
    support_lbl->setWordWrap(true);
    support_lbl->setAlignment(Qt::AlignCenter);
    support_lbl->setOpenExternalLinks(true);
    layout->addWidget(support_lbl);

    ScrollingLabel* scroll = new ScrollingLabel(patrons);
    scroll->StartAnimating();
    layout->addWidget(scroll);
  }

  layout->addWidget(new QLabel());

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
  buttons->setCenterButtons(true);
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

  setFixedSize(sizeHint());
}

}
