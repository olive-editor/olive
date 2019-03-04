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

#include "oliveglobal.h"

#include "panels/panels.h"

#include "ui/sourcetable.h"
#include "mainwindow.h"

LoadDialog::LoadDialog(QWidget *parent, const QString& fn, bool autorecovery, bool clear) :
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
  connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(cancel()));

  hboxLayout = new QHBoxLayout();
  hboxLayout->addStretch();
  hboxLayout->addWidget(cancel_button);
  hboxLayout->addStretch();

  layout->addLayout(hboxLayout);

  update();

  lt = new LoadThread(fn, autorecovery, clear);
  QObject::connect(lt, SIGNAL(success()), this, SLOT(thread_done()));
  QObject::connect(lt, SIGNAL(error()), this, SLOT(die()));
  QObject::connect(lt, SIGNAL(report_progress(int)), bar, SLOT(setValue(int)));
  lt->start();
}

void LoadDialog::cancel() {
  lt->cancel();
  lt->wait();
  die();
}

void LoadDialog::die() {
  olive::Global->new_project();
  reject();
}

void LoadDialog::thread_done() {
  accept();
}
