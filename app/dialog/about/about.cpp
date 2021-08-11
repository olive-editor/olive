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
#include "config/config.h"
#include "patreon.h"
#include "scrollinglabel.h"

#include "widget/marker/marker.h"

namespace olive {

AboutDialog::AboutDialog(bool welcome_dialog, QWidget *parent) :
  QDialog(parent)
{
  if (welcome_dialog) {
    setWindowTitle(tr("Welcome to %1").arg(QApplication::applicationName()));
  } else {
    setWindowTitle(tr("About %1").arg(QApplication::applicationName()));
  }

  QFontMetrics fm = fontMetrics();

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setMargin(fm.height());

  QHBoxLayout *horiz_layout = new QHBoxLayout();
  horiz_layout->setMargin(fm.height());
  horiz_layout->setSpacing(fm.height()*2);

  QLabel* icon = new QLabel(QStringLiteral("<html><img src=':/graphics/olive-splash.png'></html>"));
  icon->setAlignment(Qt::AlignCenter);
  horiz_layout->addWidget(icon);

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
  label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  label->setWordWrap(true);
  label->setOpenExternalLinks(true);
  label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  horiz_layout->addWidget(label);

  Marker* marker = new Marker();
  horiz_layout->addWidget(marker);

  layout->addLayout(horiz_layout);

  // Patrons where possible
  layout->addWidget(new QLabel());

  QString opening_statement;

  if (welcome_dialog || patrons.isEmpty()) {
    opening_statement = tr("<b>Olive relies on support from the community to continue its development.</b>");
  } else {
    opening_statement = tr("Olive wouldn't be possible without the support of gracious donations from the following people.");
  }

  QLabel* support_lbl = new QLabel(tr("<html>%1 "
                                        "If you like this project, please consider making a "
                                        "<a href='https://olivevideoeditor.org/donate.php'>one-time donation</a> or "
                                        "<a href='https://www.patreon.com/olivevideoeditor'>pledging monthly</a> to "
                                        "support its development.</html>").arg(opening_statement));
  support_lbl->setWordWrap(true);
  support_lbl->setAlignment(Qt::AlignCenter);
  support_lbl->setOpenExternalLinks(true);
  layout->addWidget(support_lbl);

  if (!patrons.isEmpty()) {
    ScrollingLabel* scroll = new ScrollingLabel(patrons);
    scroll->StartAnimating();
    layout->addWidget(scroll);
  }

  layout->addWidget(new QLabel());

  QHBoxLayout *btn_layout = new QHBoxLayout();
  btn_layout->setMargin(0);
  btn_layout->setSpacing(0);

  if (welcome_dialog) {
    dont_show_again_checkbox_ = new QCheckBox(tr("Don't show this message again"));
    btn_layout->addWidget(dont_show_again_checkbox_);
  } else {
    dont_show_again_checkbox_ = nullptr;
  }

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
  if (!welcome_dialog) {
    buttons->setCenterButtons(true);
  }
  btn_layout->addWidget(buttons);

  layout->addLayout(btn_layout);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

  setFixedSize(sizeHint());
}

void AboutDialog::accept()
{
  if (dont_show_again_checkbox_ && dont_show_again_checkbox_->isChecked()) {
    Config::Current()[QStringLiteral("ShowWelcomeDialog")] = false;
  }

  QDialog::accept();
}

}
