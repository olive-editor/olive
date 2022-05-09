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

#include "textv3.h"

#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QTextDocument>

#include "common/functiontimer.h"
#include "common/html.h"
#include "node/project/project.h"

namespace olive {

#define super ShapeNodeBase

enum TextVerticalAlign {
  kVerticalAlignTop,
  kVerticalAlignCenter,
  kVerticalAlignBottom,
};

const QString TextGeneratorV3::kTextInput = QStringLiteral("text_in");

TextGeneratorV3::TextGeneratorV3() :
  ShapeNodeBase(false)
{
  AddInput(kTextInput, NodeValue::kText, QStringLiteral("<p style='font-size: 72pt; color: white;'>%1</p>").arg(tr("Sample Text")));

  SetStandardValue(kSizeInput, QVector2D(400, 300));

  text_gizmo_ = new TextGizmo(this);
  text_gizmo_->SetInput(NodeInput(this, kTextInput));
}

QString TextGeneratorV3::Name() const
{
  return tr("Text");
}

QString TextGeneratorV3::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.text3");
}

QVector<Node::CategoryID> TextGeneratorV3::Category() const
{
  return {kCategoryGenerator};
}

QString TextGeneratorV3::Description() const
{
  return tr("Generate rich text.");
}

void TextGeneratorV3::Retranslate()
{
  super::Retranslate();

  SetInputName(kTextInput, tr("Text"));
}

void TextGeneratorV3::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  GenerateJob job;
  job.InsertValue(value);
  job.SetAlphaChannelRequired(GenerateJob::kAlphaForceOn);
  job.SetRequestedFormat(VideoParams::kFormatUnsigned8);

  // FIXME: Provide user override for this
  job.SetColorspace(project()->color_manager()->GetDefaultInputColorSpace());

  if (!job.GetValue(kTextInput).toString().isEmpty()) {
    PushMergableJob(value, QVariant::fromValue(job), table);
  } else if (value[kBaseInput].toTexture()) {
    table->Push(value[kBaseInput]);
  }
}

void TextGeneratorV3::GenerateFrame(FramePtr frame, const GenerateJob& job) const
{
  QImage img(reinterpret_cast<uchar*>(frame->data()), frame->width(), frame->height(), frame->linesize_bytes(), QImage::Format_RGBA8888_Premultiplied);
  img.fill(Qt::transparent);

  // 96 DPI in DPM (96 / 2.54 * 100)
  const int dpm = 3780;
  img.setDotsPerMeterX(dpm);
  img.setDotsPerMeterY(dpm);

  QTextDocument text_doc;
  text_doc.documentLayout()->setPaintDevice(&img);

  QString html = job.GetValue(kTextInput).toString();
  Html::HtmlToDoc(&text_doc, html);

  QVector2D size = job.GetValue(kSizeInput).toVec2();
  text_doc.setTextWidth(size.x());

  // Draw rich text onto image
  QPainter p(&img);
  p.scale(1.0 / frame->video_params().divider(), 1.0 / frame->video_params().divider());

  QVector2D pos = job.GetValue(kPositionInput).toVec2();
  p.translate(pos.x() - size.x()/2, pos.y() - size.y()/2);
  p.translate(frame->video_params().width()/2, frame->video_params().height()/2);
  p.setClipRect(0, 0, size.x(), size.y());

  // Ensure default text color is white
  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.palette.setColor(QPalette::Text, Qt::white);

  text_doc.documentLayout()->draw(&p, ctx);
}

void TextGeneratorV3::UpdateGizmoPositions(const NodeValueRow &row, const NodeGlobals &globals)
{
  super::UpdateGizmoPositions(row, globals);

  QRectF rect = poly_gizmo()->GetPolygon().boundingRect();
  text_gizmo_->SetRect(rect);
  text_gizmo_->SetHtml(row[kTextInput].toString());
}

}
