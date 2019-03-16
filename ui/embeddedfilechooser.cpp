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

#include "embeddedfilechooser.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QPushButton>
#include <QFileDialog>

EmbeddedFileChooser::EmbeddedFileChooser(QWidget* parent) : QWidget(parent) {
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setMargin(0);
  file_label = new QLabel(this);
  update_label();
  layout->addWidget(file_label);
  QPushButton* browse_button = new QPushButton("...", this);
  browse_button->setFixedWidth(25);
  layout->addWidget(browse_button);
  connect(browse_button, SIGNAL(clicked(bool)), this, SLOT(browse()));
}

const QString &EmbeddedFileChooser::getFilename() {
  return filename;
}

void EmbeddedFileChooser::setFilename(const QString &s) {
  filename = s;
  update_label();
  emit changed(filename);
}

void EmbeddedFileChooser::update_label() {
  QString l = "<html>" + tr("File:") + " ";
  if (filename.isEmpty()) {
    l += "(none)";
  } else {
    bool file_exists = QFileInfo::exists(filename);
    if (!file_exists) l += "<font color='red'>";
    QString short_fn = filename.mid(filename.lastIndexOf('/')+1);
    if (short_fn.size() > 20) {
      l += "..." + short_fn.right(20);
    } else {
      l += short_fn;
    }
    if (!file_exists) l += "</font>";
  }
  l += "</html>";
  file_label->setText(l);
}

void EmbeddedFileChooser::browse() {
  QString fn = QFileDialog::getOpenFileName(this);
  if (!fn.isEmpty()) {
    setFilename(fn);
  }
}
