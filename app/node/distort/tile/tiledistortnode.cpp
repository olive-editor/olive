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

#include "tiledistortnode.h"

#include "widget/slider/floatslider.h"

namespace olive {

const QString TileDistortNode::kTextureInput = QStringLiteral("tex_in");
const QString TileDistortNode::kScaleInput = QStringLiteral("scale_in");
const QString TileDistortNode::kPositionInput = QStringLiteral("position_in");
const QString TileDistortNode::kAnchorInput = QStringLiteral("anchor_in");
const QString TileDistortNode::kMirrorXInput = QStringLiteral("mirrorx_in");
const QString TileDistortNode::kMirrorYInput = QStringLiteral("mirrory_in");

#define super Node

TileDistortNode::TileDistortNode()
{
  AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));

  AddInput(kScaleInput, NodeValue::kFloat, 0.5);
  SetInputProperty(kScaleInput, QStringLiteral("min"), 0);
  SetInputProperty(kScaleInput, QStringLiteral("view"), FloatSlider::kPercentage);

  AddInput(kPositionInput, NodeValue::kVec2, QVector2D(0, 0));

  AddInput(kAnchorInput, NodeValue::kCombo, kMiddleCenter);

  AddInput(kMirrorXInput, NodeValue::kBoolean, false);
  AddInput(kMirrorYInput, NodeValue::kBoolean, false);

  SetFlag(kVideoEffect);
  SetEffectInput(kTextureInput);

  gizmo_ = AddDraggableGizmo<PointGizmo>({
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 0),
    NodeKeyframeTrackReference(NodeInput(this, kPositionInput), 1),
  });
  gizmo_->SetShape(PointGizmo::kAnchorPoint);
}

QString TileDistortNode::Name() const
{
  return tr("Tile");
}

QString TileDistortNode::id() const
{
  return QStringLiteral("org.oliveeditor.Olive.tile");
}

QVector<Node::CategoryID> TileDistortNode::Category() const
{
  return {kCategoryDistort};
}

QString TileDistortNode::Description() const
{
  return tr("Infinitely tile an image horizontally and vertically.");
}

void TileDistortNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextureInput, tr("Input"));
  SetInputName(kScaleInput, tr("Scale"));
  SetInputName(kPositionInput, tr("Position"));
  SetInputName(kMirrorXInput, tr("Mirror Horizontally"));
  SetInputName(kMirrorYInput, tr("Mirror Vertically"));

  SetInputName(kAnchorInput, tr("Anchor"));
  SetComboBoxStrings(kAnchorInput, {
    tr("Top-Left"),
    tr("Top-Center"),
    tr("Top-Right"),
    tr("Middle-Left"),
    tr("Middle-Center"),
    tr("Middle-Right"),
    tr("Bottom-Left"),
    tr("Bottom-Center"),
    tr("Bottom-Right"),
  });
}

ShaderCode TileDistortNode::GetShaderCode(const ShaderRequest &request) const
{
  Q_UNUSED(request)
  return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/tile.frag"));
}

void TileDistortNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // If there's no texture, no need to run an operation
  if (TexturePtr tex = value[kTextureInput].toTexture()) {
    // Only run shader if at least one of flip or flop are selected
    if (!qFuzzyCompare(value[kScaleInput].toDouble(), 1.0)) {
      ShaderJob job(value);
      job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));
      table->Push(NodeValue::kTexture, tex->toJob(job), this);
    } else {
      // If we're not flipping or flopping just push the texture
      table->Push(value[kTextureInput]);
    }
  }
}

void TileDistortNode::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  if (TexturePtr tex = row[kTextureInput].toTexture()) {
    QPointF res = tex->virtual_resolution().toPointF();
    QPointF pos = row[kPositionInput].toVec2().toPointF();
    qreal x = pos.x();
    qreal y = pos.y();

    Anchor a = static_cast<Anchor>(row[kAnchorInput].toInt());
    if (a == kTopLeft || a == kTopCenter || a == kTopRight) {
      // Do nothing
    } else if (a == kMiddleLeft || a == kMiddleCenter || a == kMiddleRight) {
      y += res.y()/2;
    } else if (a == kBottomLeft || a == kBottomCenter || a == kBottomRight) {
      y += res.y();
    }
    if (a == kTopLeft || a == kMiddleLeft || a == kBottomLeft) {
      // Do nothing
    } else if (a == kTopCenter || a == kMiddleCenter || a == kBottomCenter) {
      x += res.x()/2;
    } else if (a == kTopRight || a == kMiddleRight || a == kBottomRight) {
      x += res.x();
    }

    gizmo_->SetPoint(QPointF(x, y));
  }
}

void TileDistortNode::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  NodeInputDragger &x_drag = gizmo_->GetDraggers()[0];
  NodeInputDragger &y_drag = gizmo_->GetDraggers()[1];

  x_drag.Drag(x_drag.GetStartValue().toDouble() + x);
  y_drag.Drag(y_drag.GetStartValue().toDouble() + y);
}

}
