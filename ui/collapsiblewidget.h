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

class CollapsibleWidgetHeader : public QWidget {
  Q_OBJECT
public:
  CollapsibleWidgetHeader(QWidget* parent = nullptr);

  bool IsSelected();
  void SetSelected(bool s, bool deselect_others = false);

  void SetSelectable(bool s);
protected:
  virtual bool event(QEvent *event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void paintEvent(QPaintEvent *event) override;
signals:
  void select();
private:
  bool selected_;
  bool selectable_;
};

class CollapsibleWidget : public QWidget
{
  Q_OBJECT
public:
  CollapsibleWidget(QWidget* parent = nullptr);
  void SetContents(QWidget* c);
  QString Title();
  void SetTitle(const QString &);
  bool IsFocused();
  bool IsExpanded();
  void SetExpanded(bool s);

  bool IsSelected();
  void Deselect();
  void SetSelectable(bool s);
protected:
  QCheckBox* enabled_check;
  CollapsibleWidgetHeader* title_bar;
  QWidget* contents;
private:
  QLabel* header;
  QVBoxLayout* layout;
  QPushButton* collapse_button;
  QFrame* line;
  QHBoxLayout* title_bar_layout;
  void set_button_icon(bool open);

signals:
  void deselect_others(QWidget*);
  void visibleChanged(bool);

private slots:
  void on_visible_change();

public slots:
  void Selected();
};

#endif // COLLAPSIBLEWIDGET_H
