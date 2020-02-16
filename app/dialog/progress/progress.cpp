#include "progress.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ProgressDialog::ProgressDialog(const QString& message, const QString& title, QWidget *parent) :
  QDialog(parent)
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

  QHBoxLayout* cancel_layout = new QHBoxLayout();
  layout->addLayout(cancel_layout);
  cancel_layout->setMargin(0);
  cancel_layout->setSpacing(0);
  cancel_layout->addStretch();

  QPushButton* cancel_btn = new QPushButton(tr("Cancel"));
  connect(cancel_btn, &QPushButton::clicked, this, &ProgressDialog::Cancelled);
  cancel_layout->addWidget(cancel_btn);

  cancel_layout->addStretch();
}

void ProgressDialog::SetProgress(int value)
{
  bar_->setValue(value);
}
