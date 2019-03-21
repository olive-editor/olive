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

#include "loaddialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "global/global.h"

#include "panels/panels.h"

#include "ui/sourcetable.h"
#include "ui/mainwindow.h"

LoadDialog::LoadDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowTitle(tr("Loading..."));
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(tr("Loading '%1'...").arg(olive::ActiveProjectFilename.mid(olive::ActiveProjectFilename.lastIndexOf('/')+1)), this));

  bar = new QProgressBar(this);
  bar->setValue(0);
  layout->addWidget(bar);

  cancel_button = new QPushButton(tr("Cancel"), this);
  connect(cancel_button, SIGNAL(clicked(bool)), this, SIGNAL(cancel()));

  hboxLayout = new QHBoxLayout();
  hboxLayout->addStretch();
  hboxLayout->addWidget(cancel_button);
  hboxLayout->addStretch();

  layout->addLayout(hboxLayout);
}

QProgressBar *LoadDialog::progress_bar()
{
  return bar;
}
