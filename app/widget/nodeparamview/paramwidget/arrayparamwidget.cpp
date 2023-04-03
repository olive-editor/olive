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

#include "arrayparamwidget.h"

#include "widget/nodeparamview/nodeparamviewarraywidget.h"

namespace olive {

ArrayParamWidget::ArrayParamWidget(Node *node, const QString &input, QObject *parent)
  : AbstractParamWidget{parent},
    node_(node),
    input_(input)
{

}

void ArrayParamWidget::Initialize(QWidget *parent, size_t channels)
{
  NodeParamViewArrayWidget* w = new NodeParamViewArrayWidget(node_, input_, parent);
  connect(w, &NodeParamViewArrayWidget::DoubleClicked, this, &ArrayParamWidget::DoubleClicked);
  AddWidget(w);
}

}
