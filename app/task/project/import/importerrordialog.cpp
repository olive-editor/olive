/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "importerrordialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

namespace olive {

ProjectImportErrorDialog::ProjectImportErrorDialog(const QStringList& filenames, QWidget* parent) :
  QDialog(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  setWindowTitle(tr("Import Error"));

  layout->addWidget(new QLabel(tr("The following files failed to import. Olive likely does not "
                                  "support their formats.")));

  QListWidget* list_widget = new QListWidget();
  foreach (const QString& s, filenames) {
    list_widget->addItem(s);
  }
  layout->addWidget(list_widget);

  QDialogButtonBox* buttons = new QDialogButtonBox();
  buttons->setStandardButtons(QDialogButtonBox::Ok);
  buttons->setCenterButtons(true);
  connect(buttons, &QDialogButtonBox::accepted, this, &ProjectImportErrorDialog::accept);
  layout->addWidget(buttons);
}

}
