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
#include "node/project/project.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "node/generator/animation/textanimationxmlparser.h"
#include "node/generator/animation/textanimationnode.h"

#include <QDebug>  // TODO_


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
const QString TextGeneratorV3::kAnimatorsInput = QStringLiteral("animators_in");

// TODO_ comment
bool TextGeneratorV3::editing_;


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

  AddInput( kAnimatorsInput, NodeValue::kText,
            QStringLiteral("<!-- Don't write here. Attach a TextAnimation node -->"),
            InputFlags(kInputFlagNotKeyframable));

  text_gizmo_ = new TextGizmo(this);
  text_gizmo_->SetInput(NodeInput(this, kTextInput));
  connect(text_gizmo_, &TextGizmo::Activated, this, &TextGeneratorV3::GizmoActivated);
  connect(text_gizmo_, &TextGizmo::Deactivated, this, &TextGeneratorV3::GizmoDeactivated);

  engine_ = new TextAnimationEngine();
  editing_ = false;
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
  SetInputName(kAnimatorsInput, tr("Animators"));
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
    text_params.set_format(VideoParams::kFormatUnsigned8);
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

  qDebug() << "editing: " << editing_;

  if (editing_) {
    text_doc.documentLayout()->draw(&p, ctx);
  }
  else {
    int textSize = text_doc.characterCount();

    QTextCursor cursor( &text_doc);
    cursor.movePosition( QTextCursor::Start);
    cursor.movePosition( QTextCursor::Right);

    // 'kAnimatorsInput' holds a list of XML tags without main opening and closing tags.
    // Prepend and append XML start and closing tags.
    QString animators_xml = QStringLiteral("<TR>\n") +
        job.Get(kAnimatorsInput).toString() +
        QStringLiteral("</TR>");

    QList<TextAnimation::Descriptor> animators = TextAnimationXmlParser::Parse( animators_xml);

    engine_->SetTextSize( textSize);
    engine_->SetAnimators( & animators);
    engine_->Calulate();

    int posx = 0;
    int char_size = 0;
    int line_Voffset = CalculateLineHeight(cursor);


    const QVector<double> & horiz_offsets = engine_->HorizontalOffset();
    const QVector<double> & vert_offsets = engine_->VerticalOffset();
    const QVector<double> & rotations = engine_->Rotation();
    const QVector<double> & spacings = engine_->Spacing();
    const QVector<double> & horiz_stretches = engine_->HorizontalStretch();
    const QVector<double> & vert_stretches = engine_->VerticalStretch();
    const QVector<int> & transparencies = engine_->Transparency();

    // parse the whole text document and print every character
    for (int i=0; i < textSize; i++) {
      QChar ch = text_doc.characterAt(i);

      if ((ch == QChar::LineSeparator) ||
          (ch == QChar::ParagraphSeparator) )
      {
        posx = 0;
        cursor.movePosition( QTextCursor::Right);
        line_Voffset += CalculateLineHeight(cursor);
      }
      else {

        p.save();
        p.setFont( cursor.charFormat().font());
        QColor ch_color = cursor.charFormat().foreground().color();
        // "transparencies" can be assumed to be in range 0 - 255
        ch_color.setAlpha(255 - transparencies[i]);
        p.setPen( ch_color);
        p.translate( QPointF(posx + horiz_offsets[i],
                             line_Voffset + vert_offsets[i]));
        p.rotate( rotations[i]);
        p.scale( 1.+ horiz_stretches[i], 1.+ vert_stretches[i]);
        p.drawText( QPointF(0,0), ch);
        p.restore();

        char_size = QFontMetrics( cursor.charFormat().font()).horizontalAdvance(text_doc.characterAt(i));
        posx += (int)(((double)char_size*(1. + (spacings[i])))*(1. + horiz_stretches[i]));
        cursor.movePosition( QTextCursor::Right);
      }
    }
  }
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

  editing_ = true;
}

void TextGeneratorV3::GizmoDeactivated()
{
  SetStandardValue(kUseArgsInput, true);
  disconnect(text_gizmo_, &TextGizmo::VerticalAlignmentChanged, this, &TextGeneratorV3::SetVerticalAlignmentUndoable);
  dont_emit_valign_ = true;

  editing_ = false;
}

void TextGeneratorV3::SetVerticalAlignmentUndoable(Qt::Alignment a)
{
  Core::instance()->undo_stack()->push(new NodeParamSetStandardValueCommand(NodeInput(this, kVerticalAlignmentInput), GetOurAlignmentFromQts(a)));
}

int TextGeneratorV3::CalculateLineHeight(QTextCursor& start) const
{
  QTextCursor cur(start);
  int height = QFontMetrics(cur.charFormat().font()).lineSpacing();
  cur.movePosition( QTextCursor::Right);

  while (cur.atBlockEnd() ==  false) {
    int new_height = QFontMetrics(cur.charFormat().font()).lineSpacing();

    if (new_height > height) {
      height = new_height;
    }

    cur.movePosition( QTextCursor::Right);
  }

  return height;
}

}
