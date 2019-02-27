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

#ifndef COLLAPSIBLEWIDGET_H
#define COLLAPSIBLEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QIcon>

#include "ui/checkboxex.h"

class CollapsibleWidgetHeader : public QWidget {
  Q_OBJECT
public:
  CollapsibleWidgetHeader(QWidget* parent = 0);
  bool selected;
protected:
  void mousePressEvent(QMouseEvent* event);
  void paintEvent(QPaintEvent *event);
signals:
  void select(bool, bool);
};

class CollapsibleWidget : public QWidget
{
  Q_OBJECT
public:
  CollapsibleWidget(QWidget* parent = 0);
  void setContents(QWidget* c);
  void setText(const QString &);
  bool is_focused();
  bool is_expanded();

  CheckboxEx* enabled_check;
  bool selected;
  QWidget* contents;
  CollapsibleWidgetHeader* title_bar;
private:
  QLabel* header;
  QVBoxLayout* layout;
  QPushButton* collapse_button;
  QFrame* line;
  QHBoxLayout* title_bar_layout;
  void set_button_icon(bool open);

  QIcon tri_right_;
  QIcon tri_down_;

signals:
  void deselect_others(QWidget*);
  void visibleChanged();

private slots:
  void on_enabled_change(bool b);
  void on_visible_change();

public slots:
  void header_click(bool s, bool deselect);
};

#endif // COLLAPSIBLEWIDGET_H
