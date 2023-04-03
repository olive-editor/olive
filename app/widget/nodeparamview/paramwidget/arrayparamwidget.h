/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef ARRAYPARAMWIDGET_H
#define ARRAYPARAMWIDGET_H

#include "abstractparamwidget.h"

namespace olive {

class ArrayParamWidget : public AbstractParamWidget
{
  Q_OBJECT
public:
  explicit ArrayParamWidget(Node *node, const QString &input, QObject *parent = nullptr);

  virtual void Initialize(QWidget *parent, size_t channels) override;

  virtual void SetValue(const value_t &val) override {}

signals:
  void DoubleClicked();

private:
  Node *node_;
  QString input_;

};

}

#endif // ARRAYPARAMWIDGET_H
