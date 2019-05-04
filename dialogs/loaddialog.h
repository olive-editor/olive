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

/**
 * @brief The LoadDialog class
 *
 * Shows a modal dialog for loading a project. Designed to be connected to a LoadThread object. This dialog should
 * generally not be created directly, use OliveGlobal::LoadProject (or its variants) to correctly set up a LoadDialog
 * and LoadThread and connect them to each other.
 */
class LoadDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief LoadDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   */
  LoadDialog(QWidget* parent);

public slots:
  /**
   * @brief Set the progress bar value
   *
   * Ideally, connect this to LoadThread::report_progress().
   *
   * @param i
   *
   * Should be a value between 0-100.
   */
  void setValue(int i);
signals:
  /**
   * @brief Signal emitted when the cancel button is clicked.
   *
   * Ideally, connect this to LoadThread::cancel();
   */
  void cancel();
private:
  /**
   * @brief Progress bar widget
   */
  QProgressBar* bar;
};

#endif // LOADDIALOG_H
