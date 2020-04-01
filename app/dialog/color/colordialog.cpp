#include "colordialog.h"

#include <QDialogButtonBox>
#include <QSplitter>
#include <QVBoxLayout>

#include "widget/colorwheel/colorwheelwidget.h"

ColorDialog::ColorDialog(ColorManager* color_manager, QWidget *parent) :
  QDialog(parent),
  color_manager_(color_manager)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QSplitter* splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  ColorWheelWidget* cww1 = new ColorWheelWidget();
  ColorWheelWidget* cww2 = new ColorWheelWidget();

  splitter->addWidget(cww1);
  splitter->addWidget(cww2);

  QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  layout->addWidget(buttons);
}
