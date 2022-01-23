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
#include <QPainterPath>
#include <QVector2D>

#include "common/cpuoptimize.h"

namespace olive {

const QString PolygonGenerator::kPointsInput = QStringLiteral("points_in");
const QString PolygonGenerator::kColorInput = QStringLiteral("color_in");

PolygonGenerator::PolygonGenerator()
{
  AddInput(kPointsInput, NodeValue::kVec2, QVector2D(0, 0), InputFlags(kInputFlagArray));

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
  SetSplitStandardValueOnTrack(kPointsInput, 0, -kMiddleX, 1);
  SetSplitStandardValueOnTrack(kPointsInput, 1, -kMiddleY, 1);
  SetSplitStandardValueOnTrack(kPointsInput, 0, -kBottomX, 2);
  SetSplitStandardValueOnTrack(kPointsInput, 1, kBottomY, 2);
  SetSplitStandardValueOnTrack(kPointsInput, 0, kBottomX, 3);
  SetSplitStandardValueOnTrack(kPointsInput, 1, kBottomY, 3);
  SetSplitStandardValueOnTrack(kPointsInput, 0, kMiddleX, 4);
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

  QPainterPath path;

  QVector<NodeValue> points = job.GetValue(kPointsInput).data().value< QVector<NodeValue> >();

  if (!points.isEmpty()) {
    path.moveTo(points.first().data().value<QVector2D>().toPointF());

    // TODO: Implement bezier
    for (int i=1; i<points.size(); i++) {
      path.lineTo(points.at(i).data().value<QVector2D>().toPointF());
    }
  }

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

/*void PolygonGenerator::DrawGizmos(NodeValueDatabase &db, QPainter *p) const
{
  Q_UNUSED(viewport)

  if (!points_input_->GetSize()) {
    return;
  }

  p->setPen(Qt::white);
  p->setBrush(Qt::white);

  QVector<QPointF> points = GetGizmoCoordinates(db, scale);
  QVector<QRectF> rects = GetGizmoRects(points);

  points.append(points.first());

  p->drawPolyline(points.constData(), points.size());
  p->drawRects(rects);
}

bool PolygonGenerator::GizmoPress(NodeValueDatabase &db, const QPointF &p, const QVector2D &scale, const QSize& viewport)
{
  Q_UNUSED(viewport)

  QVector<QPointF> points = GetGizmoCoordinates(db, scale);
  QVector<QRectF> rects = GetGizmoRects(points);

  for (int i=0;i<rects.size();i++) {
    const QRectF& r = rects.at(i);

    if (r.contains(p)) {
      gizmo_drag_ = points_input_->At(i);
      gizmo_drag_start_ = points.at(i);
      return true;
    }
  }

  return false;
}

void PolygonGenerator::GizmoMove(const QPointF &p, const QVector2D &scale, const rational& time)
{
  QVector2D new_pos = QVector2D(p) / scale;

  if (!gizmo_x_dragger_.IsStarted()) {
    gizmo_x_dragger_.Start(gizmo_drag_, time, 0);
  }

  if (!gizmo_y_dragger_.IsStarted()) {
    gizmo_y_dragger_.Start(gizmo_drag_, time, 1);
  }

  gizmo_x_dragger_.Drag(new_pos.x());
  gizmo_y_dragger_.Drag(new_pos.y());
}

void PolygonGenerator::GizmoRelease()
{
  gizmo_x_dragger_.End();
  gizmo_y_dragger_.End();
}*/

QVector<QPointF> PolygonGenerator::GetGizmoCoordinates(NodeValueDatabase &db, const QVector2D& scale) const
{
  // FIXME: Should Get() use a `kArray` type instead of a `kVec2` type?
  QVector<NodeValueTable> array_tbl = db[kPointsInput].Get(NodeValue::kVec2).value< QVector<NodeValueTable> >();
  QVector<QPointF> points(array_tbl.size());

  for (int i=0;i<array_tbl.size();i++) {
    QVector2D v = array_tbl.at(i).Get(NodeValue::kVec2).value<QVector2D>();

    v *= scale;

    QPointF pt = v.toPointF();
    points[i] = pt;
  }

  return points;
}

QVector<QRectF> PolygonGenerator::GetGizmoRects(const QVector<QPointF> &points) const
{
  QVector<QRectF> rects(points.size());

  int rect_sz = QFontMetrics(qApp->font()).height() / 8;

  for (int i=0;i<points.size();i++) {
    const QPointF& p = points.at(i);

    rects[i] = QRectF(p - QPointF(rect_sz, rect_sz),
                      p + QPointF(rect_sz, rect_sz));
  }

  return rects;
}

}
