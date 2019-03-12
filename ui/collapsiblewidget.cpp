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

#include "collapsiblewidget.h"

#include <QDebug>
#include <QLabel>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QWidget>
#include <QPainter>

#include "ui/checkboxex.h"
#include "ui/icons.h"

#include "debug.h"

CollapsibleWidget::CollapsibleWidget(QWidget* parent) : QWidget(parent) {
  selected = false;

  layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  title_bar = new CollapsibleWidgetHeader(this);
  title_bar->setFocusPolicy(Qt::ClickFocus);
  title_bar->setAutoFillBackground(true);
  title_bar_layout = new QHBoxLayout(title_bar);
  title_bar_layout->setMargin(5);
  enabled_check = new CheckboxEx(title_bar);
  enabled_check->setChecked(true);
  header = new QLabel(title_bar);
  collapse_button = new QPushButton(title_bar);
  collapse_button->setIconSize(collapse_button->iconSize()*0.5);
  collapse_button->setStyleSheet("QPushButton { border: none; }");
  SetText(tr("<untitled>"));
  title_bar_layout->addWidget(collapse_button);
  title_bar_layout->addWidget(enabled_check);
  title_bar_layout->addWidget(header);
  title_bar_layout->addStretch();
  layout->addWidget(title_bar);

  connect(title_bar, SIGNAL(select(bool, bool)), this, SLOT(header_click(bool, bool)));

  set_button_icon(true);

  contents = nullptr;
}

void CollapsibleWidget::header_click(bool s, bool deselect) {
  selected = s;
  title_bar->selected = s;
  if (s) {
    QPalette p = title_bar->palette();
    p.setColor(QPalette::Background, QColor(255, 255, 255, 64));
    title_bar->setPalette(p);
  } else {
    title_bar->setPalette(palette());
  }
  if (deselect) emit deselect_others(this);
}

bool CollapsibleWidget::IsFocused() {
  if (hasFocus()) return true;
  return title_bar->hasFocus();
}

bool CollapsibleWidget::IsExpanded() {
  return contents->isVisible();
}

bool CollapsibleWidget::IsSelected()
{
  return selected;
}

void CollapsibleWidget::set_button_icon(bool open) {
  collapse_button->setIcon(open ? olive::icon::DownArrow : olive::icon::RightArrow);
}

void CollapsibleWidget::SetContents(QWidget* c) {
  bool existing = (contents != nullptr);
  contents = c;
  if (!existing) {
    layout->addWidget(contents);
    connect(enabled_check, SIGNAL(toggled(bool)), this, SLOT(on_enabled_change(bool)));
    connect(collapse_button, SIGNAL(clicked()), this, SLOT(on_visible_change()));
  }
}

void CollapsibleWidget::SetText(const QString &s) {
  header->setText(s);
}

void CollapsibleWidget::on_enabled_change(bool b) {
  contents->setEnabled(b);
}

void CollapsibleWidget::on_visible_change() {
  contents->setVisible(!contents->isVisible());
  set_button_icon(contents->isVisible());
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
  int line_y = height() - 1;
  p.drawLine(0, line_y, width(), line_y);
}
