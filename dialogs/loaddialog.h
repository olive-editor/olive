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

#ifndef LOADDIALOG_H
#define LOADDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QHBoxLayout>

#include "project/projectelements.h"
#include "project/loadthread.h"

class LoadDialog : public QDialog
{
  Q_OBJECT
public:
  LoadDialog(QWidget* parent, const QString& filename, bool autorecovery, bool clear);
private slots:
  void cancel();
  void die();
  void thread_done();
private:
  QProgressBar* bar;
  QPushButton* cancel_button;
  QHBoxLayout* hboxLayout;
  LoadThread* lt;
};

#endif // LOADDIALOG_H
