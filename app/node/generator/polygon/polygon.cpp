/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "polygon.h"

#include <QVector2D>

OLIVE_NAMESPACE_ENTER

PolygonGenerator::PolygonGenerator()
{
  points_input_ = new NodeInputArray("points_in", NodeParam::kVec2);
  AddInput(points_input_);

  color_input_ = new NodeInput("color_in", NodeParam::kColor);
  AddInput(color_input_);

  // Default to "a color" that isn't
  color_input_->set_standard_value(1.0, 0);
  color_input_->set_standard_value(1.0, 1);
  color_input_->set_standard_value(1.0, 2);
  color_input_->set_standard_value(1.0, 3);

  // FIXME: Test code
  points_input_->SetSize(5);
  points_input_->At(0)->set_standard_value(960, 0);
  points_input_->At(0)->set_standard_value(240, 1);
  points_input_->At(1)->set_standard_value(640, 0);
  points_input_->At(1)->set_standard_value(480, 1);
  points_input_->At(2)->set_standard_value(760, 0);
  points_input_->At(2)->set_standard_value(800, 1);
  points_input_->At(3)->set_standard_value(1100, 0);
  points_input_->At(3)->set_standard_value(800, 1);
  points_input_->At(4)->set_standard_value(1280, 0);
  points_input_->At(4)->set_standard_value(480, 1);
  // End test
}

Node *PolygonGenerator::copy() const
{
  return new PolygonGenerator();
}

QString PolygonGenerator::Name() const
{
  return tr("Polygon");
}

QString PolygonGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.polygon");
}

QList<Node::CategoryID> PolygonGenerator::Category() const
{
  return {kCategoryGenerator};
}

QString PolygonGenerator::Description() const
{
  return tr("Generate a 2D polygon of any amount of points.");
}

void PolygonGenerator::Retranslate()
{
  points_input_->set_name(tr("Points"));
  color_input_->set_name(tr("Color"));
}

Node::Capabilities PolygonGenerator::GetCapabilities(const NodeValueDatabase &) const
{
  return kShader;
}

QString PolygonGenerator::ShaderFragmentCode(const NodeValueDatabase &) const
{
  return Node::ReadFileAsString(":/shaders/polygon.frag");
}

bool PolygonGenerator::HasGizmos() const
{
  return true;
}

void PolygonGenerator::DrawGizmos(NodeValueDatabase &db, QPainter *p, const QVector2D &scale) const
{
  if (!points_input_->GetSize()) {
    return;
  }

  QVector<QPointF> points(points_input_->GetSize());

  p->setPen(Qt::white);
  p->setBrush(Qt::white);

  int rect_sz = p->fontMetrics().height() / 8;

  for (int i=0;i<points_input_->GetSize();i++) {
    QVector2D v = db[points_input_->At(i)].Take(NodeParam::kVec2).value<QVector2D>();

    v *= scale;

    QPointF pt = v.toPointF();
    points[i] = pt;

    QRectF rect(pt - QPointF(rect_sz, rect_sz),
                pt + QPointF(rect_sz, rect_sz));
    p->drawRect(rect);
  }

  points.append(points.first());

  p->drawPolyline(points.constData(), points.size());
}

OLIVE_NAMESPACE_EXIT
