#include "loaddialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

LoadDialog::LoadDialog(QWidget *parent) : QDialog(parent) {
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QVBoxLayout* layout = new QVBoxLayout();
	setLayout(layout);

	layout->addWidget(new QLabel("Loading ''..."));

	bar = new QProgressBar();
	bar->setValue(50);
	layout->addWidget(bar);

	QPushButton* cancel_button = new QPushButton("Cancel");
	connect(cancel_button, SIGNAL(clicked(bool)), this, SLOT(reject()));

	QHBoxLayout* hboxLayout = new QHBoxLayout();
	hboxLayout->addStretch();
	hboxLayout->addWidget(cancel_button);
	hboxLayout->addStretch();

	layout->addLayout(hboxLayout);
}
