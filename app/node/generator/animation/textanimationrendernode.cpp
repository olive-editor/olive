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

#include "textanimationrendernode.h"

#include <QGuiApplication>
#include <QVector2D>
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include <QTextCursor>

#include "common/cpuoptimize.h"
#include "common/html.h"
#include "node/generator/animation/textanimationxmlparser.h"
#include "node/generator/text/textv3.h"


namespace olive {

const QString TextAnimationRenderNode::kRichTextInput = QStringLiteral("rich_text_in");
const QString TextAnimationRenderNode::kAnimatorsInput = QStringLiteral("animators_in");

const QString kCurrentTime = QStringLiteral("time_now");

#define super Node

TextAnimationRenderNode::TextAnimationRenderNode()
{
  AddInput( kRichTextInput, NodeValue::kText,
            QStringLiteral("<p style='font-size: 72pt; color: white;'>%1</p>").arg(tr("Sample Text")),
            InputFlags(kInputFlagNotKeyframable));
  AddInput( kAnimatorsInput, NodeValue::kText,
            QStringLiteral("<!-- Don't write here. Attach a TextAnimation node -->"),
            InputFlags(kInputFlagNotKeyframable));

  SetEffectInput(kRichTextInput);

  engine_ = new TextAnimationEngine();
}

QString TextAnimationRenderNode::Name() const
{
  return tr("Text Animation Render");
}

QString TextAnimationRenderNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.TextAnimationRenderNode");
}

QVector<Node::CategoryID> TextAnimationRenderNode::Category() const
{
  return {kCategoryGenerator};
}

QString TextAnimationRenderNode::Description() const
{
  return tr("Takes a rich text and a set of animation descriptors to produce the text animation");
}

void TextAnimationRenderNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kRichTextInput, tr("Text"));
  SetInputName(kAnimatorsInput, tr("Animators"));
}

GenerateJob TextAnimationRenderNode::GetGenerateJob(const NodeValueRow &value) const
{
  GenerateJob job;

  job.Insert(value);
  job.SetRequestedFormat(VideoParams::kFormatUnsigned8);

  return job;
}

void TextAnimationRenderNode::Value(const NodeValueRow &value, const NodeGlobals & globals, NodeValueTable *table) const
{
  GenerateJob job = GetGenerateJob(value);
  // We store here the current time that will be used in 'GenerateFrame'
  job.Insert(kCurrentTime, NodeValue(NodeValue::kRational, globals.time().in(), this));

  table->Push(NodeValue::kTexture, job, this);
}


void TextAnimationRenderNode::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  QImage img(reinterpret_cast<uchar*>(frame->data()), frame->width(), frame->height(),
             frame->linesize_bytes(), QImage::Format_RGBA8888_Premultiplied);
  img.fill(Qt::transparent);

  Node * textConnectedNode = GetConnectedOutput( kRichTextInput);
  rational time = job.Get(kCurrentTime).toRational();

  // 96 DPI in DPM (96 / 2.54 * 100)
  const int dpm = 3780;
  img.setDotsPerMeterX(dpm);
  img.setDotsPerMeterY(dpm);

  QTextDocument text_doc;
  text_doc.setDefaultFont(QFont("Arial", 50));

  // default value if no "Text" node is connected
  QString html = job.Get(kRichTextInput).toString();
  if (textConnectedNode != nullptr) {
    html = textConnectedNode->GetValueAtTime( TextGeneratorV3::kTextInput, time).toString();
  }
  Html::HtmlToDoc(&text_doc, html);
  QTextCursor cursor( &text_doc);

  // aspect ratio
  double par = frame->video_params().pixel_aspect_ratio().toDouble();
  QPointF scale_factor = QPointF(1.0 / frame->video_params().divider() / par, 1.0 / frame->video_params().divider());

  QPainter p(&img);
  p.scale( scale_factor.x(), scale_factor.y());

  QVector2D pos(200.,200.);
  if (textConnectedNode != nullptr) {
    // Use input "Text" node to define the base position of the text
    pos = GetConnectedOutput( kRichTextInput)->GetValueAtTime( ShapeNodeBase::kPositionInput, time).value<QVector2D>();
    QVector2D size = GetConnectedOutput( kRichTextInput)->GetValueAtTime( ShapeNodeBase::kSizeInput, time).value<QVector2D>();
    p.translate(pos.x() - size.x()/2, pos.y() - size.y()/2);
    p.translate(frame->video_params().width()/2, frame->video_params().height()/2);
  }

  p.setRenderHints( QPainter::TextAntialiasing | QPainter::LosslessImageRendering |
                    QPainter::SmoothPixmapTransform);

  int textSize = text_doc.characterCount();
  cursor.movePosition( QTextCursor::Start);
  cursor.movePosition( QTextCursor::Right);

  // 'kAnimatorsInput' holds a list of XML tags without main opening and closing tags.
  // Prepend and append XML start and closing tags.
  QString animators_xml = QStringLiteral("<TR>\n") + job.Get(kAnimatorsInput).toString() +
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

int TextAnimationRenderNode::CalculateLineHeight(QTextCursor& start) const
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


}  // olive
