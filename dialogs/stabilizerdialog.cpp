#include "stabilizerdialog.h"

#include <QVBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>

StabilizerDialog::StabilizerDialog(QWidget *parent) : QDialog(parent) {
    layout = new QVBoxLayout(this);
    setLayout(layout);

    enable_stab = new QCheckBox(this);
    enable_stab->setText("Enable Stabilizer");
    layout->addWidget(enable_stab);

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}
