/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef NODEPARAMVIEWARRAYWIDGET_H
#define NODEPARAMVIEWARRAYWIDGET_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "node/param.h"

namespace olive {

class NodeParamViewArrayButton : public QPushButton
{
  Q_OBJECT
public:
  enum Type {
    kAdd,
    kRemove
  };

  NodeParamViewArrayButton(Type type, QWidget* parent = nullptr);

protected:
  virtual void changeEvent(QEvent* event) override;

private:
  void Retranslate();

  Type type_;

};

class NodeParamViewArrayWidget : public QWidget
{
  Q_OBJECT
public:
  NodeParamViewArrayWidget(Node* node, const QString& input, QWidget* parent = nullptr);

signals:
  void DoubleClicked();

protected:
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
  Node* node_;

  QString input_;

  QLabel* count_lbl_;

private slots:
  void UpdateCounter(const QString &input, int old_size, int new_size);

};

}

#endif // NODEPARAMVIEWARRAYWIDGET_H
