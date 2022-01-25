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
  SetInputName(kPointsInput, tr("Points"));
  SetInputName(kColorInput, tr("Color"));
}

void PolygonGenerator::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  GenerateJob job;

  job.InsertValue(value);
  job.SetRequestedFormat(VideoParams::kFormatFloat32);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);

  table->Push(NodeValue::kGenerateJob, QVariant::fromValue(job), this);
}

void PolygonGenerator::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  // This could probably be more optimized, but for now we use Qt to draw to a QImage.
  // QImages only support integer pixels and we use float pixels, so what we do here is draw onto
  // a single-channel QImage (alpha only) and then transplant that alpha channel to our float buffer
  // with correct float RGB.
  QImage img(frame->width(), frame->height(), QImage::Format_Grayscale8);
  img.fill(Qt::transparent);

  QVector<NodeValue> points = job.GetValue(kPointsInput).data().value< QVector<NodeValue> >();

  QPainterPath path = GeneratePath(points);

  QPainter p(&img);
  double par = frame->video_params().pixel_aspect_ratio().toDouble();
  p.scale(1.0 / frame->video_params().divider() / par, 1.0 / frame->video_params().divider());
  p.translate(frame->video_params().width()/2 * par, frame->video_params().height()/2);
  p.setBrush(Qt::white);
  p.setPen(Qt::NoPen);

  p.drawPath(path);

  // Transplant alpha channel to frame
  Color rgba = job.GetValue(kColorInput).data().value<Color>();
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

bool PolygonGenerator::HasGizmos() const
{
  return true;
}

void PolygonGenerator::DrawGizmos(const NodeValueRow &row, const NodeGlobals &globals, QPainter *p)
{
  const double handle_radius = GetGizmoHandleRadius(p->transform());
  const double bezier_radius = handle_radius/2;

  p->setPen(Qt::white);
  p->setBrush(Qt::white);
  p->translate(globals.resolution_by_par().x()/2, globals.resolution_by_par().y()/2);

  QVector<NodeValue> points = row[kPointsInput].data().value< QVector<NodeValue> >();

  gizmo_position_handles_.resize(points.size());
  gizmo_bezier_handles_.resize(points.size() * 2);

  p->setPen(QPen(Qt::white, 0));
  p->setBrush(Qt::NoBrush);

  if (!points.isEmpty()) {
    QVector<QLineF> lines(points.size() * 2);

    for (int i=0; i<points.size(); i++) {
      const Bezier &pt = points.at(i).data().value<Bezier>();

      QPointF main = pt.ToPointF();
      QPointF cp1 = main + pt.ControlPoint1ToPointF();
      QPointF cp2 = main + pt.ControlPoint2ToPointF();

      gizmo_position_handles_[i] = CreateGizmoHandleRect(main, handle_radius);

      gizmo_bezier_handles_[i*2] = CreateGizmoHandleRect(cp1, bezier_radius);
      lines[i*2] = QLineF(main, cp1);

      gizmo_bezier_handles_[i*2+1] = CreateGizmoHandleRect(cp2, bezier_radius);
      lines[i*2+1] = QLineF(main, cp2);
    }

    p->drawLines(lines);
  }

  gizmo_polygon_path_ = GeneratePath(points);
  p->drawPath(gizmo_polygon_path_);

  DrawAndExpandGizmoHandles(p, handle_radius, gizmo_position_handles_.data(), gizmo_position_handles_.size());
  DrawAndExpandGizmoHandles(p, handle_radius, gizmo_bezier_handles_.data(), gizmo_bezier_handles_.size());
}

bool PolygonGenerator::GizmoPress(const NodeValueRow& row, const NodeGlobals &globals, const QPointF &p)
{
  QPointF adjusted = p - (globals.resolution_by_par() / 2).toPointF();

  // First, look for main points

  for (int i=0; i<gizmo_position_handles_.size(); i++) {
    if (gizmo_position_handles_.at(i).contains(adjusted)) {
      gizmo_x_active_.append(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 0));
      gizmo_y_active_.append(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 1));
      break;
    }
  }

  // Next, if no main points were found, look for beziers
  if (gizmo_x_active_.isEmpty() && gizmo_y_active_.isEmpty()) {
    for (int i=0; i<gizmo_bezier_handles_.size(); i++) {
      if (gizmo_bezier_handles_.at(i).contains(adjusted)) {
        int start = (i%2 == 0) ? 2 : 4;
        int element = i/2;
        gizmo_x_active_.append(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, element), start + 0));
        gizmo_y_active_.append(NodeKeyframeTrackReference(NodeInput(this, kPointsInput, element), start + 1));
        break;
      }
    }

    // Finally, see if the cursor is inside the polygon
    if (gizmo_x_active_.isEmpty() && gizmo_y_active_.isEmpty()) {
      if (gizmo_polygon_path_.contains(adjusted)) {
        gizmo_x_active_.resize(gizmo_position_handles_.size());
        gizmo_y_active_.resize(gizmo_position_handles_.size());
        for (int i=0; i<gizmo_position_handles_.size(); i++) {
          gizmo_x_active_[i] = NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 0);
          gizmo_y_active_[i] = NodeKeyframeTrackReference(NodeInput(this, kPointsInput, i), 1);
        }
      }
    }
  }

  gizmo_drag_start_ = p;

  return !gizmo_x_active_.isEmpty() || !gizmo_y_active_.isEmpty();
}

void PolygonGenerator::GizmoMove(const QPointF &p, const rational &time, const Qt::KeyboardModifiers &modifiers)
{
  if (gizmo_x_draggers_.isEmpty() && gizmo_y_draggers_.isEmpty()) {
    gizmo_x_draggers_.resize(gizmo_x_active_.size());
    gizmo_y_draggers_.resize(gizmo_y_active_.size());
    for (int i=0; i<gizmo_x_active_.size(); i++) {
      gizmo_x_draggers_[i].Start(gizmo_x_active_.at(i), time);
    }
    for (int i=0; i<gizmo_y_active_.size(); i++) {
      gizmo_y_draggers_[i].Start(gizmo_y_active_.at(i), time);
    }
  }

  QPointF diff = p - gizmo_drag_start_;

  for (NodeInputDragger &dragger : gizmo_x_draggers_) {
    dragger.Drag(dragger.GetStartValue().toDouble() + diff.x());
  }

  for (NodeInputDragger &dragger : gizmo_y_draggers_) {
    dragger.Drag(dragger.GetStartValue().toDouble() + diff.y());
  }
}

void PolygonGenerator::GizmoRelease(MultiUndoCommand *command)
{
  for (NodeInputDragger &dragger : gizmo_x_draggers_) {
    dragger.End(command);
  }
  gizmo_x_draggers_.clear();

  for (NodeInputDragger &dragger : gizmo_y_draggers_) {
    dragger.End(command);
  }
  gizmo_y_draggers_.clear();

  gizmo_x_active_.clear();
  gizmo_y_active_.clear();
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
    const Bezier &first_pt = points.first().data().value<Bezier>();
    path.moveTo(first_pt.ToPointF());

    for (int i=1; i<points.size(); i++) {
      AddPointToPath(&path, points.at(i-1).data().value<Bezier>(), points.at(i).data().value<Bezier>());
    }

    AddPointToPath(&path, points.last().data().value<Bezier>(), first_pt);
  }

  return path;
}

}
