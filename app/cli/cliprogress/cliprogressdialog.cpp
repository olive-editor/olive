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

#include "cliprogressdialog.h"

#include <iostream>

OLIVE_NAMESPACE_ENTER

CLIProgressDialog::CLIProgressDialog(const QString& title, QObject *parent) :
  QObject(parent),
  title_(title),
  progress_(-1),
  drawn_(false)
{
  SetProgress(0);
}

void CLIProgressDialog::Update()
{
  if (drawn_) {
    // We've been here before, do a carriage return back to the start of the terminal line
    std::cout << "\r";
  } else {
    drawn_ = true;
  }

  // FIXME: Get real column count
  int columns = 80;

  int title_columns = columns / 2 - 1;

  // Print "title" text
  QString sized_title = title_;

  if (title_.size() > title_columns) {
    sized_title = title_.left(title_columns - 3).append(QStringLiteral("..."));
  } else {
    sized_title = title_;
  }

  std::cout << sized_title.toUtf8().constData();

  // Pad out the rest of the title area if necessary
  for (int i=sized_title.size(); i<title_columns; i++) {
    std::cout << " ";
  }

  // Percentage counter " 100% " is 5 characters + the enclosing brackets [] are 2 characters
  int progress_bar_columns = columns / 2 - 7;

  std::cout << "[";

  // Get UI bar progress
  int bar_prog = qRound(progress_ * progress_bar_columns);

  // Draw filled in bar
  for (int i=0;i<bar_prog;i++) {
    std::cout << "=";
  }

  // Draw empty space
  for (int i=bar_prog;i<progress_bar_columns;i++) {
    std::cout << " ";
  }

  std::cout << "] ";

  if (progress_ < 100) {
    std::cout << " ";
  }

  if (progress_ < 10) {
    std::cout << " ";
  }

  std::cout << qRound(progress_ * 100.0) << "% " << std::endl << std::flush;
}

void CLIProgressDialog::SetProgress(double p)
{
  if (progress_ != p) {
    progress_ = p;

    Update();
  }
}

OLIVE_NAMESPACE_EXIT
