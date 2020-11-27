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

#include "progress.h"

#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "window/mainwindow/mainwindow.h"

namespace olive {

ProgressDialog::ProgressDialog(const QString& message, const QString& title, QWidget *parent) :
  QDialog(parent),
  show_progress_(true)
{
  if (!title.isEmpty()) {
    setWindowTitle(title);
  }

  QVBoxLayout* layout = new QVBoxLayout(this);

  layout->addWidget(new QLabel(message));

  bar_ = new QProgressBar();
  bar_->setMinimum(0);
  bar_->setValue(0);
  bar_->setMaximum(100);
  layout->addWidget(bar_);

  elapsed_timer_lbl_ = new ElapsedCounterWidget();
  layout->addWidget(elapsed_timer_lbl_);

  QHBoxLayout* cancel_layout = new QHBoxLayout();
  layout->addLayout(cancel_layout);
  cancel_layout->setMargin(0);
  cancel_layout->setSpacing(0);
  cancel_layout->addStretch();

  QPushButton* cancel_btn = new QPushButton(tr("Cancel"));

  // Signal that derivatives can connect to
  connect(cancel_btn, &QPushButton::clicked, this, &ProgressDialog::Cancelled, Qt::DirectConnection);

  // Stop updating the elapsed/remaining timers
  connect(cancel_btn, &QPushButton::clicked, elapsed_timer_lbl_, &ElapsedCounterWidget::Stop);

  // Disable the button so that users know they don't need to keep clicking it
  connect(cancel_btn, &QPushButton::clicked, this, &ProgressDialog::DisableSenderWidget);

  // Prevent the progress bar from continuing to move
  connect(cancel_btn, &QPushButton::clicked, this, &ProgressDialog::DisableProgressWidgets);

  cancel_layout->addWidget(cancel_btn);

  cancel_layout->addStretch();
}

void ProgressDialog::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);

  elapsed_timer_lbl_->Start();

  Core::instance()->main_window()->SetApplicationProgressStatus(MainWindow::kProgressShow);
}

void ProgressDialog::closeEvent(QCloseEvent *e)
{
  QDialog::closeEvent(e);

  Core::instance()->main_window()->SetApplicationProgressStatus(MainWindow::kProgressNone);
}

void ProgressDialog::SetProgress(double value)
{
  if (!show_progress_) {
    return;
  }

  int percent = qRound(100.0 * value);

  bar_->setValue(percent);
  elapsed_timer_lbl_->SetProgress(value);

  Core::instance()->main_window()->SetApplicationProgressValue(percent);
}

void ProgressDialog::ShowErrorMessage(const QString &title, const QString &message)
{
  Core::instance()->main_window()->SetApplicationProgressStatus(MainWindow::kProgressError);

  QMessageBox b(this);
  b.setIcon(QMessageBox::Critical);
  b.setWindowModality(Qt::WindowModal);
  b.setWindowTitle(title);
  b.setText(message);
  b.addButton(QMessageBox::Ok);
  b.exec();
}

void ProgressDialog::DisableSenderWidget()
{
  static_cast<QWidget*>(sender())->setEnabled(false);
}

void ProgressDialog::DisableProgressWidgets()
{
  show_progress_ = false;
}

}
