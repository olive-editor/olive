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

#include "polygon.h"

#include <QGuiApplication>
#include <QVector2D>

namespace olive {

const QString PolygonGenerator::kPointsInput = QStringLiteral("points_in");
const QString PolygonGenerator::kColorInput = QStringLiteral("color_in");

#define super GeneratorWithMerge

PolygonGenerator::PolygonGenerator()
{
  AddInput(kPointsInput, NodeValue::kBezier, QVector2D(0, 0), InputFlags(kInputFlagArray));

  AddInput(kColorInput, NodeValue::kColor, QVariant::fromValue(Color(1.0, 1.0, 1.0)));

  const int kMiddleX = 135;
  const int kMiddleY = 45;
  const int kBottomX = 90;
  const int kBottomY = 120;
  const int kTopY = 135;

  // The Default Pentagon(tm)
  InputArrayResize(kPointsInput, 5);
  SetSplitStandardValueOnTrack(kPointsInput, 0, 0, 0);
  SetSplitStandardValueOnTrack(kPointsInput, 1, -kTopY, 0);
  SetSplitStandardValueOnTrack(kPointsInput, 0, kMiddleX, 1);
  SetSplitStandardValueOnTrack(kPointsInput, 1, -kMiddleY, 1);
  SetSplitStandardValueOnTrack(kPointsInput, 0, kBottomX, 2);
  SetSplitStandardValueOnTrack(kPointsInput, 1, kBottomY, 2);
  SetSplitStandardValueOnTrack(kPointsInput, 0, -kBottomX, 3);
  SetSplitStandardValueOnTrack(kPointsInput, 1, kBottomY, 3);
  SetSplitStandardValueOnTrack(kPointsInput, 0, -kMiddleX, 4);
  SetSplitStandardValueOnTrack(kPointsInput, 1, -kMiddleY, 4);

  // Initiate gizmos
  poly_gizmo_ = new PathGizmo(this);
}

QString PolygonGenerator::Name() const
{
  return tr("Polygon");
}

QString PolygonGenerator::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.polygon");
}

QVector<Node::CategoryID> PolygonGenerator::Category() const
{
  return {kCategoryGenerator};
}

QString PolygonGenerator::Description() const
{
  return tr("Generate a 2D polygon of any amount of points.");
}

void PolygonGenerator::Retranslate()
{
  super::Retranslate();

  SetInputName(kPointsInput, tr("Points"));
  SetInputName(kColorInput, tr("Color"));
}

ShaderJob PolygonGenerator::GetGenerateJob(const NodeValueRow &value, const VideoParams &params) const
{
  VideoParams p = params;
  p.set_format(PixelFormat::U8);
  auto job = Texture::Job(p, GenerateJob(value));

  // Conversion to RGB
  ShaderJob rgb;
  rgb.SetShaderID(QStringLiteral("rgb"));
  rgb.Insert(QStringLiteral("texture_in"), NodeValue(NodeValue::kTexture, job, this));
  rgb.Insert(QStringLiteral("color_in"), value[kColorInput]);

  return rgb;
}

void PolygonGenerator::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  PushMergableJob(value, Texture::Job(globals.vparams(), GetGenerateJob(value, globals.vparams())), table);
}

void PolygonGenerator::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  // This could probably be more optimized, but for now we use Qt to draw to a QImage.
  // QImages only support integer pixels and we use float pixels, so what we do here is draw onto
  // a single-channel QImage (alpha only) and then transplant that alpha channel to our float buffer
  // with correct float RGB.
  QImage img((uchar *) frame->data(), frame->width(), frame->height(), frame->linesize_bytes(), QImage::Format_RGBA8888_Premultiplied);
  img.fill(Qt::transparent);

  auto points = job.Get(kPointsInput).toArray();

  QPainterPath path = GeneratePath(points, InputArraySize(kPointsInput));

  QPainter p(&img);
  double par = frame->video_params().pixel_aspect_ratio().toDouble();
  p.scale(1.0 / frame->video_params().divider() / par, 1.0 / frame->video_params().divider());
  p.translate(frame->video_params().width()/2 * par, frame->video_params().height()/2);
  p.setBrush(Qt::white);
  p.setPen(Qt::NoPen);

  p.drawPath(path);
}

template<typename T>
NodeGizmo *PolygonGenerator::CreateAppropriateGizmo()
{
  return new T(this);
}

template<>
NodeGizmo *PolygonGenerator::CreateAppropriateGizmo<PointGizmo>()
{
  return AddDraggableGizmo<PointGizmo>();
}

template<typename T>
void PolygonGenerator::ValidateGizmoVectorSize(QVector<T*> &vec, int new_sz)
{
  int old_sz = vec.size();

  if (old_sz != new_sz) {
    if (old_sz > new_sz) {
      for (int i=new_sz; i<old_sz; i++) {
        delete vec.at(i);
      }
    }

    vec.resize(new_sz);

    if (old_sz < new_sz) {
      for (int i=old_sz; i<new_sz; i++) {
        vec[i] = static_cast<T*>(CreateAppropriateGizmo<T>());
      }
    }
  }
}

void PolygonGenerator::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  QVector2D res;
  if (TexturePtr tex = row[kBaseInput].toTexture()) {
    res = tex->virtual_resolution();
  } else {
    res = globals.square_resolution();
  }

  Imath::V2d half_res(res.x()/2, res.y()/2);

  auto points = row[kPointsInput].toArray();

  int current_pos_sz = gizmo_position_handles_.size();

  ValidateGizmoVectorSize(gizmo_position_handles_, points.size());
  ValidateGizmoVectorSize(gizmo_bezier_handles_, points.size() * 2);
  ValidateGizmoVectorSize(gizmo_bezier_lines_, points.size() * 2);

  for (int i=current_pos_sz; i<gizmo_position_handles_.size(); i++) {
    gizmo_position_handles_.at(i)->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 0));
    gizmo_position_handles_.at(i)->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 1));

    PointGizmo *bez_gizmo1 = gizmo_bezier_handles_.at(i*2+0);
    bez_gizmo1->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 2));
    bez_gizmo1->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 3));
    bez_gizmo1->SetShape(PointGizmo::kCircle);
    bez_gizmo1->SetSmaller(true);

    PointGizmo *bez_gizmo2 = gizmo_bezier_handles_.at(i*2+1);
    bez_gizmo2->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 4));
    bez_gizmo2->AddInput(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 5));
    bez_gizmo2->SetShape(PointGizmo::kCircle);
    bez_gizmo2->SetSmaller(true);
  }

  int pts_sz = InputArraySize(kPointsInput);
  if (!points.empty()) {
    for (int i=0; i<pts_sz; i++) {
      const Bezier &pt = points.at(i).toBezier();

      Imath::V2d main = pt.to_vec() + half_res;
      Imath::V2d cp1 = main + pt.control_point_1_to_vec();
      Imath::V2d cp2 = main + pt.control_point_2_to_vec();

      gizmo_position_handles_[i]->SetPoint(QPointF(main.x, main.y));

      gizmo_bezier_handles_[i*2]->SetPoint(QPointF(cp1.x, cp1.y));
      gizmo_bezier_lines_[i*2]->SetLine(QLineF(QPointF(main.x, main.y), QPointF(cp1.x, cp1.y)));
      gizmo_bezier_handles_[i*2+1]->SetPoint(QPointF(cp2.x, cp2.y));
      gizmo_bezier_lines_[i*2+1]->SetLine(QLineF(QPointF(main.x, main.y), QPointF(cp2.x, cp2.y)));
    }
  }

  poly_gizmo_->SetPath(GeneratePath(points, pts_sz).translated(QPointF(half_res.x, half_res.y)));
}

ShaderCode PolygonGenerator::GetShaderCode(const ShaderRequest &request) const
{
  if (request.id == QStringLiteral("rgb")) {
    return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/rgb.frag"));
  } else {
    return super::GetShaderCode(request);
  }
}

void PolygonGenerator::GizmoDragMove(double x, double y, const Qt::KeyboardModifiers &modifiers)
{
  DraggableGizmo *gizmo = static_cast<DraggableGizmo*>(sender());

  if (gizmo == poly_gizmo_) {
    // FIXME: Drag all points
  } else {
    NodeInputDragger &x_drag = gizmo->GetDraggers()[0];
    NodeInputDragger &y_drag = gizmo->GetDraggers()[1];
    x_drag.Drag(x_drag.GetStartValue().toDouble() + x);
    y_drag.Drag(y_drag.GetStartValue().toDouble() + y);
  }
}

void PolygonGenerator::AddPointToPath(QPainterPath *path, const Bezier &before, const Bezier &after)
{
  Imath::V2d a = before.to_vec() + before.control_point_2_to_vec();
  Imath::V2d b = after.to_vec() + after.control_point_1_to_vec();
  Imath::V2d c = after.to_vec();

  path->cubicTo(QPointF(a.x, a.y), QPointF(b.x, b.y), QPointF(c.x, c.y));
}

QPainterPath PolygonGenerator::GeneratePath(const NodeValueArray &points, int size)
{
  QPainterPath path;

  if (!points.empty()) {
    const Bezier &first_pt = points.at(0).toBezier();
    Imath::V2d v = first_pt.to_vec();
    path.moveTo(QPointF(v.x, v.y));

    for (int i=1; i<size; i++) {
      AddPointToPath(&path, points.at(i-1).toBezier(), points.at(i).toBezier());
    }

    AddPointToPath(&path, points.at(size-1).toBezier(), first_pt);
  }

  return path;
}

}
