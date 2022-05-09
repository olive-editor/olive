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

#include "polygon.h"

#include <QGuiApplication>
#include <QVector2D>

#include "common/cpuoptimize.h"

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

GenerateJob PolygonGenerator::GetGenerateJob(const NodeValueRow &value) const
{
  GenerateJob job;

  job.InsertValue(value);
  job.SetRequestedFormat(VideoParams::kFormatFloat32);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  return job;
}

void PolygonGenerator::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  GenerateJob job = GetGenerateJob(value);

  PushMergableJob(value, QVariant::fromValue(job), table);
}

void PolygonGenerator::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  // This could probably be more optimized, but for now we use Qt to draw to a QImage.
  // QImages only support integer pixels and we use float pixels, so what we do here is draw onto
  // a single-channel QImage (alpha only) and then transplant that alpha channel to our float buffer
  // with correct float RGB.
  QImage img(frame->width(), frame->height(), QImage::Format_Grayscale8);
  img.fill(Qt::transparent);

  QVector<NodeValue> points = job.GetValue(kPointsInput).value< QVector<NodeValue> >();

  QPainterPath path = GeneratePath(points);

  QPainter p(&img);
  double par = frame->video_params().pixel_aspect_ratio().toDouble();
  p.scale(1.0 / frame->video_params().divider() / par, 1.0 / frame->video_params().divider());
  p.translate(frame->video_params().width()/2 * par, frame->video_params().height()/2);
  p.setBrush(Qt::white);
  p.setPen(Qt::NoPen);

  p.drawPath(path);

  // Transplant alpha channel to frame
  Color rgba = job.GetValue(kColorInput).toColor();
#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
  __m128 sse_color = _mm_loadu_ps(rgba.data());
#endif

  float *frame_dst = reinterpret_cast<float*>(frame->data());
  for (int y=0; y<frame->height(); y++) {
    uchar *src_y = img.bits() + img.bytesPerLine() * y;
    float *dst_y = frame_dst + y*frame->linesize_pixels()*VideoParams::kRGBAChannelCount;

    for (int x=0; x<frame->width(); x++) {
      float alpha = float(src_y[x]) / 255.0f;
      float *dst = dst_y + x*VideoParams::kRGBAChannelCount;

#if defined(Q_PROCESSOR_X86) || defined(Q_PROCESSOR_ARM)
      __m128 sse_alpha = _mm_load1_ps(&alpha);
      __m128 sse_res = _mm_mul_ps(sse_color, sse_alpha);

      _mm_store_ps(dst, sse_res);
#else
      for (int i=0; i<VideoParams::kRGBAChannelCount; i++) {
        dst[i] = rgba.data()[i] * alpha;
      }
#endif
    }
  }
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
  QPointF half_res(globals.resolution_by_par().x()/2, globals.resolution_by_par().y()/2);

  QVector<NodeValue> points = row[kPointsInput].value< QVector<NodeValue> >();

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

  if (!points.isEmpty()) {
    for (int i=0; i<points.size(); i++) {
      const Bezier &pt = points.at(i).toBezier();

      QPointF main = pt.ToPointF() + half_res;
      QPointF cp1 = main + pt.ControlPoint1ToPointF();
      QPointF cp2 = main + pt.ControlPoint2ToPointF();

      gizmo_position_handles_[i]->SetPoint(main);

      gizmo_bezier_handles_[i*2]->SetPoint(cp1);
      gizmo_bezier_lines_[i*2]->SetLine(QLineF(main, cp1));
      gizmo_bezier_handles_[i*2+1]->SetPoint(cp2);
      gizmo_bezier_lines_[i*2+1]->SetLine(QLineF(main, cp2));
    }
  }

  poly_gizmo_->SetPath(GeneratePath(points).translated(half_res));
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
  path->cubicTo(before.ToPointF() + before.ControlPoint2ToPointF(),
                after.ToPointF() + after.ControlPoint1ToPointF(),
                after.ToPointF());
}

QPainterPath PolygonGenerator::GeneratePath(const QVector<NodeValue> &points)
{
  QPainterPath path;

  if (!points.isEmpty()) {
    const Bezier &first_pt = points.first().toBezier();
    path.moveTo(first_pt.ToPointF());

    for (int i=1; i<points.size(); i++) {
      AddPointToPath(&path, points.at(i-1).toBezier(), points.at(i).toBezier());
    }

    AddPointToPath(&path, points.last().toBezier(), first_pt);
  }

  return path;
}

}
