/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "texteffect.h"

#include <QGridLayout>
#include <QLabel>
#include <QOpenGLTexture>
#include <QTextEdit>
#include <QPainter>
#include <QPushButton>
#include <QColorDialog>
#include <QFontDatabase>
#include <QComboBox>
#include <QWidget>
#include <QtMath>
#include <QMenu>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "ui/blur.h"
#include "global/config.h"

TextEffect::TextEffect(Clip* c) :
  OldEffectNode(c)
{
  SetFlags(OldEffectNode::SuperimposeFlag);

  text_val = new StringInput(this, "text", tr("Text"), false);

  set_font_combobox = new FontInput(this, "font", tr("Font"));

  size_val = new DoubleInput(this, "size", tr("Size"));
  size_val->SetMinimum(0);

  set_color_button = new ColorInput(this, "color", tr("Color"));

  halign_field = new ComboInput(this, "halign", tr("Horizontal Alignment"));
  halign_field->AddItem(tr("Left"), Qt::AlignLeft);
  halign_field->AddItem(tr("Center"), Qt::AlignHCenter);
  halign_field->AddItem(tr("Right"), Qt::AlignRight);
  halign_field->AddItem(tr("Justify"), Qt::AlignJustify);

  valign_field = new ComboInput(this, "valign", tr("Vertical Alignment"));
  valign_field->AddItem(tr("Top"), Qt::AlignTop);
  valign_field->AddItem(tr("Center"), Qt::AlignVCenter);
  valign_field->AddItem(tr("Bottom"), Qt::AlignBottom);

  word_wrap_field = new BoolInput(this, "wordwrap", tr("Word Wrap"));

  padding_field = new DoubleInput(this, "padding", tr("Padding"));

  position = new Vec2Input(this, "pos", tr("Position"));

  outline_bool = new BoolInput(this, "outline", tr("Outline"));

  outline_color = new ColorInput(this, "outlinecolor", tr("Outline Color"));

  outline_width = new DoubleInput(this, "outlinewidth", tr("Outline Width"));
  outline_width->SetMinimum(0);

  shadow_bool = new BoolInput(this, "shadow", tr("Shadow"));

  shadow_color = new ColorInput(this, "shadowcolor", tr("Shadow Color"));

  shadow_angle = new DoubleInput(this, "shadowangle", tr("Shadow Angle"));

  shadow_distance = new DoubleInput(this, "shadowdistance", tr("Shadow Distance"));
  shadow_distance->SetMinimum(0);

  shadow_softness = new DoubleInput(this, "shadowsoftness", tr("Shadow Softness"));
  shadow_softness->SetMinimum(0);

  shadow_opacity = new DoubleInput(this, "shadowopacity", tr("Shadow Opacity"));
  shadow_opacity->SetMinimum(0);
  shadow_opacity->SetMaximum(100);

  size_val->SetDefault(48);
  text_val->SetValueAt(0, tr("Sample Text"));
  set_color_button->SetValueAt(0, QColor(Qt::white));
  halign_field->SetValueAt(0, Qt::AlignHCenter);
  valign_field->SetValueAt(0, Qt::AlignVCenter);
  word_wrap_field->SetValueAt(0, true);
  outline_color->SetValueAt(0, QColor(Qt::black));
  shadow_color->SetValueAt(0, QColor(Qt::black));
  shadow_angle->SetDefault(45);
  shadow_opacity->SetDefault(100);
  shadow_softness->SetDefault(5);
  shadow_distance->SetDefault(5);
  shadow_opacity->SetDefault(80);
  outline_width->SetDefault(20);

  outline_enable(false);
  shadow_enable(false);

  connect(shadow_bool, SIGNAL(Toggled(bool)), this, SLOT(shadow_enable(bool)));
  connect(outline_bool, SIGNAL(Toggled(bool)), this, SLOT(outline_enable(bool)));

  shader_vert_path_ = "common.vert";
  shader_frag_path_ = "dropshadow.frag";
}

QString TextEffect::name()
{
  return tr("Text");
}

QString TextEffect::id()
{
  return "org.olivevideoeditor.Olive.text";
}

QString TextEffect::category()
{
  return tr("Render");
}

QString TextEffect::description()
{
  return tr("Generate simple text over this clip");
}

EffectType TextEffect::type()
{
  return EFFECT_TYPE_EFFECT;
}

olive::TrackType TextEffect::subtype()
{
  return olive::kTypeVideo;
}

OldEffectNodePtr TextEffect::Create(Clip *c)
{
  return std::make_shared<TextEffect>(c);
}

void TextEffect::redraw(double timecode) {
  if (size_val->GetDoubleAt(timecode) <= 0) {
    return;
  }

  QColor bkg = set_color_button->GetColorAt(timecode);
  bkg.setAlpha(0);
  img.fill(bkg);

  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  int padding = qRound(padding_field->GetDoubleAt(timecode));
  int width = img.width() - padding * 2;
  int height = img.height() - padding * 2;

  // set font
  font.setStyleHint(QFont::Helvetica, QFont::PreferAntialias);
  font.setFamily(set_font_combobox->GetFontAt(timecode));
  font.setPointSize(qRound(size_val->GetDoubleAt(timecode)));
  p.setFont(font);
  QFontMetrics fm(font);

  QStringList lines = text_val->GetStringAt(timecode).split('\n');

  // word wrap function
  if (word_wrap_field->GetBoolAt(timecode)) {
    for (int i=0;i<lines.size();i++) {
      QString s(lines.at(i));
      if (fm.width(s) > width) {
        int last_space_index = 0;
        for (int j=0;j<s.length();j++) {
          if (s.at(j) == ' ') {
            if (fm.width(s.left(j)) > width) {
              break;
            } else {
              last_space_index = j;
            }
          }
        }
        if (last_space_index > 0) {
          lines.insert(i+1, s.mid(last_space_index + 1));
          lines[i] = s.left(last_space_index);
        }
      }
    }
  }

  QPainterPath path;

  int text_height = fm.height()*lines.size();

  for (int i=0;i<lines.size();i++) {
    int text_x, text_y;

    switch (halign_field->GetValueAt(timecode).toInt()) {
    case Qt::AlignLeft: text_x = 0; break;
    case Qt::AlignRight: text_x = width - fm.width(lines.at(i)); break;
    case Qt::AlignJustify:
      // add spaces until the string is too big
      text_x = 0;
      while (fm.width(lines.at(i)) < width) {
        bool space = false;
        QString spaced(lines.at(i));
        for (int i=0;i<spaced.length();i++) {
          if (spaced.at(i) == ' ') {
            // insert a space
            spaced.insert(i, ' ');
            space = true;

            // scan to next non-space
            while (i < spaced.length() && spaced.at(i) == ' ') i++;
          }
        }
        if (fm.width(spaced) > width || !space) {
          break;
        } else {
          lines[i] = spaced;
        }
      }
      break;
    case Qt::AlignHCenter:
    default:
      text_x = (width/2) - (fm.width(lines.at(i))/2);
      break;
    }

    switch (valign_field->GetValueAt(timecode).toInt()) {
    case Qt::AlignTop:
      text_y = (fm.height()*i)+fm.ascent();
      break;
    case Qt::AlignBottom:
      text_y = (height - text_height - fm.descent()) + (fm.height()*(i+1));
      break;
    case Qt::AlignVCenter:
    default:
      text_y = ((height/2) - (text_height/2) - fm.descent()) + (fm.height()*(i+1));
      break;
    }

    path.addText(text_x, text_y, font, lines.at(i));
  }

  path.translate(position->GetVector2DAt(timecode).toPointF() + QPointF(padding, padding));

  // draw software shadow
  if (shadow_bool->GetBoolAt(timecode)) {
    p.setPen(Qt::NoPen);

    // calculate offset using distance and angle
    double angle = shadow_angle->GetDoubleAt(timecode) * M_PI / 180.0;
    double distance = qFloor(shadow_distance->GetDoubleAt(timecode));
    int shadow_x_offset = qRound(qCos(angle) * distance);
    int shadow_y_offset = qRound(qSin(angle) * distance);

    QPainterPath shadow_path(path);
    shadow_path.translate(shadow_x_offset, shadow_y_offset);

    QColor col = shadow_color->GetColorAt(timecode);
    col.setAlpha(0);
    img.fill(col);

    col.setAlphaF(shadow_opacity->GetDoubleAt(timecode)*0.01);
    p.setBrush(col);
    p.drawPath(shadow_path);

    int blurSoftness = qFloor(shadow_softness->GetDoubleAt(timecode));
    if (blurSoftness > 0) olive::ui::blur(img, img.rect(), blurSoftness, true);
  }

  // draw outline
  int outline_width_val = qCeil(outline_width->GetDoubleAt(timecode));
  if (outline_bool->GetBoolAt(timecode) && outline_width_val > 0) {
    QPen outline(outline_color->GetColorAt(timecode));
    outline.setWidth(outline_width_val);
    p.setPen(outline);
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
  }

  // draw "master" text
  p.setPen(Qt::NoPen);
  p.setBrush(set_color_button->GetColorAt(timecode));
  p.drawPath(path);

  p.end();
}

void TextEffect::shadow_enable(bool e) {
  shadow_color->SetEnabled(e);
  shadow_angle->SetEnabled(e);
  shadow_distance->SetEnabled(e);
  shadow_softness->SetEnabled(e);
  shadow_opacity->SetEnabled(e);
}

void TextEffect::outline_enable(bool e) {
  outline_color->SetEnabled(e);
  outline_width->SetEnabled(e);
}
