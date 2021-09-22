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

#include "shapenodebase.h"

#include <QVector2D>

namespace olive {

#define super Node

QString ShapeNodeBase::kPositionInput = QStringLiteral("pos_in");
QString ShapeNodeBase::kSizeInput = QStringLiteral("size_in");
QString ShapeNodeBase::kColorInput = QStringLiteral("color_in");

ShapeNodeBase::ShapeNodeBase()
{
  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(0, 0));
  AddInput(kSizeInput, NodeValue::kVec2, QVector2D(100, 100));
  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0, 0.0, 0.0, 1.0)));
}

void ShapeNodeBase::Retranslate()
{
  super::Retranslate();

  SetInputName(kPositionInput, tr("Position"));
  SetInputName(kSizeInput, tr("Size"));
  SetInputName(kColorInput, tr("Color"));
}

}
