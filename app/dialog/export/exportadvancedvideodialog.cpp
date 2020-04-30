#include "exportadvancedvideodialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

OLIVE_NAMESPACE_ENTER

ExportAdvancedVideoDialog::ExportAdvancedVideoDialog(QWidget *parent) :
  QDialog(parent)
{
  setWindowTitle(tr("Advanced"));

  QGridLayout* layout = new QGridLayout(this);

  int row = 0;

  layout->addWidget(new QLabel(tr("Threads:")), row, 0);

  thread_slider_ = new IntegerSlider();
  thread_slider_->SetMinimum(0);
  layout->addWidget(thread_slider_, row, 1);

  row++;

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &ExportAdvancedVideoDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &ExportAdvancedVideoDialog::reject);
  layout->addWidget(buttons, row, 0, 1, 2);
}

int ExportAdvancedVideoDialog::threads() const
{
  return static_cast<int>(thread_slider_->GetValue());
}

void ExportAdvancedVideoDialog::set_threads(int t)
{
  thread_slider_->SetValue(t);
}

OLIVE_NAMESPACE_EXIT
