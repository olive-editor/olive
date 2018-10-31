#include "collapsiblewidget.h"

#include "ui/checkboxex.h"

#include <QDebug>
#include <QLabel>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QWidget>
#include <QPainter>

CollapsibleWidget::CollapsibleWidget(QWidget* parent) : QWidget(parent) {
    selected = false;

	layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);

	title_bar = new CollapsibleWidgetHeader();
	title_bar->setFocusPolicy(Qt::ClickFocus);
	title_bar->setAutoFillBackground(true);
    QHBoxLayout* title_bar_layout = new QHBoxLayout();
	title_bar_layout->setMargin(5);
	title_bar->setLayout(title_bar_layout);
	enabled_check = new CheckboxEx();
	enabled_check->setChecked(true);
	header = new QLabel();
	collapse_button = new QPushButton("[-]");
	collapse_button->setStyleSheet("QPushButton { border: none; } QPushButton:hover { text-decoration: underline; }");
	collapse_button->setMaximumWidth(25);
	setText("<untitled>");
    title_bar_layout->addWidget(collapse_button);
    title_bar_layout->addWidget(enabled_check);
    title_bar_layout->addWidget(header);
    title_bar_layout->addStretch();
    layout->addWidget(title_bar);

	connect(title_bar, SIGNAL(select(bool, bool)), this, SLOT(header_click(bool, bool)));


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

bool CollapsibleWidget::is_expanded() {
	return contents->isVisible();
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
	collapse_button->setText(contents->isVisible() ? "[-]" : "[+]");
	emit visibleChanged();
}

CollapsibleWidgetHeader::CollapsibleWidgetHeader(QWidget* parent) : QWidget(parent), selected(false) {
	setContextMenuPolicy(Qt::CustomContextMenu);
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

void CollapsibleWidgetHeader::paintEvent(QPaintEvent *event) {
	QWidget::paintEvent(event);
	QPainter p(this);
	p.setPen(Qt::white);
	int line_x = width() * 0.01;
	int line_y = height() - 1;
	p.drawLine(line_x, line_y, width() - line_x - line_x, line_y);
}
