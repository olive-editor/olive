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
#include <QStyle>
#include <QApplication>

#include "ui/icons.h"
#include "global/debug.h"

CollapsibleWidget::CollapsibleWidget(QWidget* parent) :
  QWidget(parent),
  contents(nullptr)
{
  layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  title_bar = new CollapsibleWidgetHeader(this);
  title_bar->setFocusPolicy(Qt::ClickFocus);
  title_bar->setAutoFillBackground(true);
  title_bar_layout = new QHBoxLayout(title_bar);
  title_bar_layout->setMargin(5);
  enabled_check = new QCheckBox(title_bar);
  enabled_check->setChecked(true);
  header = new QLabel(title_bar);
  collapse_button = new QPushButton(title_bar);
  collapse_button->setIconSize(collapse_button->iconSize()*0.5);
  collapse_button->setFlat(true);
  SetTitle(tr("<untitled>"));
  title_bar_layout->addWidget(collapse_button);
  title_bar_layout->addWidget(enabled_check);
  title_bar_layout->addWidget(header);
  title_bar_layout->addStretch();
  layout->addWidget(title_bar);

  connect(title_bar, SIGNAL(select()), this, SLOT(Selected()));

  set_button_icon(true);
}

void CollapsibleWidget::Selected() {
  emit deselect_others(this);
}

bool CollapsibleWidget::IsFocused() {
  if (hasFocus()) return true;
  return title_bar->hasFocus();
}

bool CollapsibleWidget::IsExpanded() {
  return contents->isVisible();
}

void CollapsibleWidget::SetExpanded(bool s)
{
  contents->setVisible(s);
  set_button_icon(s);
  emit visibleChanged(s);
}

bool CollapsibleWidget::IsSelected()
{
  return title_bar->IsSelected();
}

void CollapsibleWidget::Deselect()
{
  title_bar->SetSelected(false);
}

void CollapsibleWidget::SetSelectable(bool s)
{
  title_bar->SetSelectable(s);
}

void CollapsibleWidget::set_button_icon(bool open) {
  collapse_button->setIcon(open ? olive::icon::DownArrow : olive::icon::RightArrow);
}

void CollapsibleWidget::SetContents(QWidget* c) {
  bool existing = (contents != nullptr);
  contents = c;
  if (!existing) {
    layout->addWidget(contents);
    connect(collapse_button, SIGNAL(clicked()), this, SLOT(on_visible_change()));
  }
}

QString CollapsibleWidget::Title()
{
  return header->text();
}

void CollapsibleWidget::SetTitle(const QString &s) {
  header->setText(s);
}

void CollapsibleWidget::on_visible_change() {
  SetExpanded(!IsExpanded());
}

CollapsibleWidgetHeader::CollapsibleWidgetHeader(QWidget* parent) :
  QWidget(parent),
  selected_(false),
  selectable_(true)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
}

bool CollapsibleWidgetHeader::IsSelected()
{
  return selected_;
}

void CollapsibleWidgetHeader::SetSelected(bool s, bool deselect_others)
{
  selected_ = s;

  if (s) {
    QPalette p = palette();
    p.setColor(QPalette::Background, QColor(255, 255, 255, 64));
    setPalette(p);
  } else {
    setPalette(qApp->palette());
  }

  if (deselect_others) {
    emit select();
  }
}

void CollapsibleWidgetHeader::SetSelectable(bool s)
{
  selectable_ = s;

  if (!selectable_ && selected_) {
    SetSelected(false);
  }
}

bool CollapsibleWidgetHeader::event(QEvent *event)
{
  if (!selectable_
      && (event->type() == QEvent::MouseButtonPress
      || event->type() == QEvent::MouseButtonRelease
      || event->type() == QEvent::MouseMove
      || event->type() == QEvent::MouseButtonDblClick)
      && QApplication::sendEvent(parent(), event)) {
    return true;
  }
  return QWidget::event(event);
}

void CollapsibleWidgetHeader::mousePressEvent(QMouseEvent* event) {
  if (selected_) {
    if (event->modifiers() & Qt::ShiftModifier) {
      SetSelected(false);
    }
  } else {
    SetSelected(true, !(event->modifiers() & Qt::ShiftModifier));
  }
}

void CollapsibleWidgetHeader::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);
  QPainter p(this);
  p.setPen(Qt::white);
  int line_y = height() - 1;
  p.drawLine(0, line_y, width(), line_y);
}
