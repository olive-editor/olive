#include "collapsiblewidget.h"

#include <QDebug>
#include <QLabel>
#include <QCheckBox>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

CollapsibleWidget::CollapsibleWidget(QWidget* parent) : QWidget(parent)
{
	layout = new QVBoxLayout(this);
	layout->setMargin(0);

	title_bar = new QHBoxLayout();
	title_bar->setMargin(0);
	enabled_check = new QCheckBox();
	enabled_check->setChecked(true);
	header = new QLabel();
	collapse_button = new QPushButton("-");
	collapse_button->setMaximumWidth(25);
	setText("<untitled>");
	title_bar->addWidget(collapse_button);
	title_bar->addWidget(enabled_check);
	title_bar->addWidget(header);
	title_bar->addStretch();
	layout->addLayout(title_bar);

	line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	layout->addWidget(line);

	contents = NULL;
}

void CollapsibleWidget::setContents(QWidget* c) {
	bool existing = (contents != NULL);
	contents = c;
	if (!existing) {
		layout->addWidget(contents);
		connect(enabled_check, SIGNAL(toggled(bool)), this, SLOT(on_enabled_change(bool)));
		connect(collapse_button, SIGNAL(clicked()), this, SLOT(on_visible_change()));
	}
}

void CollapsibleWidget::setText(const QString &s) {
	header->setText(s);
}

void CollapsibleWidget::on_enabled_change(bool b) {
	contents->setEnabled(b);
}

void CollapsibleWidget::on_visible_change() {
	contents->setVisible(!contents->isVisible());
	collapse_button->setText(contents->isVisible() ? "-" : "+");
}
