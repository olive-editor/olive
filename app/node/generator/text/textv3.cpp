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

#include "textv3.h"

#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QTextDocument>

#include "common/html.h"
#include "core.h"
#include "node/project.h"
#include "node/nodeundo.h"

namespace olive {

#define super ShapeNodeBase

enum TextVerticalAlign {
  kVerticalAlignTop,
  kVerticalAlignCenter,
  kVerticalAlignBottom,
};

const QString TextGeneratorV3::kTextInput = QStringLiteral("text_in");
const QString TextGeneratorV3::kVerticalAlignmentInput = QStringLiteral("valign_in");
const QString TextGeneratorV3::kUseArgsInput = QStringLiteral("use_args_in");
const QString TextGeneratorV3::kArgsInput = QStringLiteral("args_in");

TextGeneratorV3::TextGeneratorV3() :
  ShapeNodeBase(false),
  dont_emit_valign_(false)
{
  AddInput(kTextInput, NodeValue::kText, QStringLiteral("<p style='font-size: 72pt; color: white;'>%1</p>").arg(tr("Sample Text")));
  SetInputProperty(kTextInput, QStringLiteral("vieweronly"), true);

  SetStandardValue(kSizeInput, QVector2D(400, 300));

  AddInput(kVerticalAlignmentInput, NodeValue::kCombo, InputFlags(kInputFlagHidden | kInputFlagStatic));

  AddInput(kUseArgsInput, NodeValue::kBoolean, true, InputFlags(kInputFlagHidden | kInputFlagStatic));

  AddInput(kArgsInput, NodeValue::kText, InputFlags(kInputFlagArray));
  SetInputProperty(kArgsInput, QStringLiteral("arraystart"), 1);

  text_gizmo_ = new TextGizmo(this);
  text_gizmo_->SetInput(NodeInput(this, kTextInput));
  connect(text_gizmo_, &TextGizmo::Activated, this, &TextGeneratorV3::GizmoActivated);
  connect(text_gizmo_, &TextGizmo::Deactivated, this, &TextGeneratorV3::GizmoDeactivated);
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
  SetInputName(kVerticalAlignmentInput, tr("Vertical Alignment"));
  SetComboBoxStrings(kVerticalAlignmentInput, {tr("Top"), tr("Middle"), tr("Bottom")});
  SetInputName(kArgsInput, tr("Arguments"));
}

void TextGeneratorV3::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  QString text = value[kTextInput].toString();

  if (value[kUseArgsInput].toBool()) {
    auto args = value[kArgsInput].toArray();
    if (!args.empty()) {
      QStringList list;
      list.reserve(args.size());
      for (size_t i=0; i<args.size(); i++) {
        list.append(args[i].toString());
      }

      text = FormatString(text, list);
    }
  }

  if (!text.isEmpty()) {
    TexturePtr base = value[kTextInput].toTexture();

    VideoParams text_params = base ? base->params() : globals.vparams();
    text_params.set_format(PixelFormat::U8);
    text_params.set_colorspace(project()->color_manager()->GetDefaultInputColorSpace());

    GenerateJob job(value);
    job.Insert(kTextInput, NodeValue(NodeValue::kText, text));

    PushMergableJob(value, Texture::Job(text_params, job), table);
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

  QString html = job.Get(kTextInput).toString();
  Html::HtmlToDoc(&text_doc, html);

  QVector2D size = job.Get(kSizeInput).toVec2();
  text_doc.setTextWidth(size.x());

  // Draw rich text onto image
  QPainter p(&img);
  p.scale(1.0 / frame->video_params().divider(), 1.0 / frame->video_params().divider());

  QVector2D pos = job.Get(kPositionInput).toVec2();
  p.translate(pos.x() - size.x()/2, pos.y() - size.y()/2);
  p.translate(frame->video_params().width()/2, frame->video_params().height()/2);
  p.setClipRect(0, 0, size.x(), size.y());

  switch (static_cast<VerticalAlignment>(job.Get(kVerticalAlignmentInput).toInt())) {
  case kVAlignTop:
    // Do nothing
    break;
  case kVAlignMiddle:
    p.translate(0, size.y()/2-text_doc.size().height()/2);
    break;
  case kVAlignBottom:
    p.translate(0, size.y()-text_doc.size().height());
    break;
  }

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

Qt::Alignment TextGeneratorV3::GetQtAlignmentFromOurs(VerticalAlignment v)
{
  switch (v) {
  case kVAlignTop:
    return Qt::AlignTop;
  case kVAlignMiddle:
    return Qt::AlignVCenter;
  case kVAlignBottom:
    return Qt::AlignBottom;
  }
  return Qt::Alignment();
}

TextGeneratorV3::VerticalAlignment TextGeneratorV3::GetOurAlignmentFromQts(Qt::Alignment v)
{
  switch (v) {
  case Qt::AlignTop:
    return kVAlignTop;
  case Qt::AlignVCenter:
    return kVAlignMiddle;
  case Qt::AlignBottom:
    return kVAlignBottom;
  }

  return kVAlignTop;
}

QString TextGeneratorV3::FormatString(const QString &input, const QStringList &args)
{
  QString output;
  output.reserve(input.size());

  for (int i=0; i<input.size(); i++) {
    const QChar &this_char = input.at(i);

    if (i < input.size()-1 && this_char == '%') {
      const QChar &next_char = input.at(i+1);
      if (next_char == '%') {
        // Double percent, append a single percent
        output.append('%');
        i++;
      } else if (next_char.isDigit()) {
        // Find length of number
        QString num;
        i++;
        while (i < input.size() && input.at(i).isDigit()) {
          num.append(input.at(i));
          i++;
        }
        i--;
        int index = num.toInt()-1;
        if (index >= 0 && index < args.size()) {
          output.append(args.at(index));
        }
      } else {
        output.append(this_char);
      }
    } else {
      output.append(this_char);
    }
  }

  return output;
}

void TextGeneratorV3::InputValueChangedEvent(const QString &input, int element)
{
  if (input == kVerticalAlignmentInput && !dont_emit_valign_) {
    text_gizmo_->SetVerticalAlignment(GetQtAlignmentFromOurs(GetVerticalAlignment()));
  }

  super::InputValueChangedEvent(input, element);
}

void TextGeneratorV3::GizmoActivated()
{
  SetStandardValue(kUseArgsInput, false);
  connect(text_gizmo_, &TextGizmo::VerticalAlignmentChanged, this, &TextGeneratorV3::SetVerticalAlignmentUndoable);
  dont_emit_valign_ = true;
}

void TextGeneratorV3::GizmoDeactivated()
{
  SetStandardValue(kUseArgsInput, true);
  disconnect(text_gizmo_, &TextGizmo::VerticalAlignmentChanged, this, &TextGeneratorV3::SetVerticalAlignmentUndoable);
  dont_emit_valign_ = true;
}

void TextGeneratorV3::SetVerticalAlignmentUndoable(Qt::Alignment a)
{
  Core::instance()->undo_stack()->push(new NodeParamSetStandardValueCommand(NodeInput(this, kVerticalAlignmentInput), GetOurAlignmentFromQts(a)), tr("Set Text Vertical Alignment"));
}

}
