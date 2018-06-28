#include "collapsiblewidget.h"

#include <QDebug>
#include <QLabel>
#include <QCheckBox>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QWidget>

CollapsibleWidget::CollapsibleWidget(QWidget* parent) : QWidget(parent) {
    selected = false;

	layout = new QVBoxLayout(this);
	layout->setMargin(0);

    title_bar = new CollapsibleWidgetHeader();
    title_bar->setFocusPolicy(Qt::ClickFocus);
    title_bar->setAutoFillBackground(true);
    QHBoxLayout* title_bar_layout = new QHBoxLayout();
    title_bar->setLayout(title_bar_layout);
    title_bar_layout->setMargin(0);
	enabled_check = new QCheckBox();
	enabled_check->setChecked(true);
	header = new QLabel();
	collapse_button = new QPushButton("-");
	collapse_button->setMaximumWidth(25);
	setText("<untitled>");
    title_bar_layout->addWidget(collapse_button);
    title_bar_layout->addWidget(enabled_check);
    title_bar_layout->addWidget(header);
    title_bar_layout->addStretch();
    layout->addWidget(title_bar);

    connect(title_bar, SIGNAL(select(bool, bool)), this, SLOT(header_click(bool, bool)));

	line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
	layout->addWidget(line);

	contents = NULL;
}

void CollapsibleWidget::header_click(bool s, bool deselect) {
    selected = s;
    title_bar->selected = s;
    if (s) {
        QPalette p = palette();
        p.setColor(QPalette::Background, QColor(255, 255, 255, 64));
        title_bar->setPalette(p);
    } else {
        title_bar->setPalette(palette());
    }
    if (deselect) emit deselect_others(this);
}

bool CollapsibleWidget::is_focused() {
    if (hasFocus()) return true;
    return title_bar->hasFocus();
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

CollapsibleWidgetHeader::CollapsibleWidgetHeader(QWidget* parent) : QWidget(parent) {
    selected = false;
}

void CollapsibleWidgetHeader::mousePressEvent(QMouseEvent* event) {
    if (selected) {
        if ((event->modifiers() & Qt::ShiftModifier)) {
            selected = false;
            emit select(selected, false);
        }
    } else {
        selected = true;
        emit select(selected, !(event->modifiers() & Qt::ShiftModifier));
    }
}
