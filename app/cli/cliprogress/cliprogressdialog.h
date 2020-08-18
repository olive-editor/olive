/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef CLIPROGRESSDIALOG_H
#define CLIPROGRESSDIALOG_H

#include <QObject>
#include <QTimer>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class CLIProgressDialog : public QObject
{
public:
  CLIProgressDialog(const QString &title, QObject* parent = nullptr);

public slots:
  void SetProgress(double p);

private:
  void Update();

  QString title_;

  double progress_;

  bool drawn_;

};

OLIVE_NAMESPACE_EXIT

#endif // CLIPROGRESSDIALOG_H
