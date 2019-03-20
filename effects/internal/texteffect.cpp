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

TextEffect::TextEffect(Clip* c, const EffectMeta* em) :
  Effect(c, em)
{
  SetFlags(Effect::SuperimposeFlag);

  EffectRow* text_field = new EffectRow(this, tr("Text"));
  text_val = new StringField(text_field, "text", false);
  text_val->SetColumnSpan(2);

  EffectRow* font_row = new EffectRow(this, tr("Font"));
  set_font_combobox = new FontField(font_row, "font");
  set_font_combobox->SetColumnSpan(2);

  EffectRow* size_row = new EffectRow(this, tr("Size"));
  size_val = new DoubleField(size_row, "size");
  size_val->SetMinimum(0);
  size_val->SetColumnSpan(2);

  EffectRow* color_row = new EffectRow(this, tr("Color"));
  set_color_button = new ColorField(color_row, "color");
  set_color_button->SetColumnSpan(2);

  EffectRow* alignment_row = new EffectRow(this, tr("Alignment"));
  halign_field = new ComboField(alignment_row, "halign");
  halign_field->AddItem(tr("Left"), Qt::AlignLeft);
  halign_field->AddItem(tr("Center"), Qt::AlignHCenter);
  halign_field->AddItem(tr("Right"), Qt::AlignRight);
  halign_field->AddItem(tr("Justify"), Qt::AlignJustify);

  valign_field = new ComboField(alignment_row, "valign");
  valign_field->AddItem(tr("Top"), Qt::AlignTop);
  valign_field->AddItem(tr("Center"), Qt::AlignVCenter);
  valign_field->AddItem(tr("Bottom"), Qt::AlignBottom);

  EffectRow* word_wrap_row = new EffectRow(this, tr("Word Wrap"));
  word_wrap_field = new BoolField(word_wrap_row, "wordwrap");
  word_wrap_field->SetColumnSpan(2);

  EffectRow* padding_row = new EffectRow(this, tr("Padding"));
  padding_field = new DoubleField(padding_row, "padding");
  padding_field->SetColumnSpan(2);

  EffectRow* position_row = new EffectRow(this, tr("Position"));
  position_x = new DoubleField(position_row, "posx");
  position_y = new DoubleField(position_row, "posy");

  EffectRow* outline_row = new EffectRow(this, tr("Outline"));
  outline_bool = new BoolField(outline_row, "outline");
  outline_bool->SetColumnSpan(2);

  EffectRow* outline_color_row = new EffectRow(this, tr("Outline Color"));
  outline_color = new ColorField(outline_color_row, "outlinecolor");
  outline_color->SetColumnSpan(2);

  EffectRow* outline_width_row = new EffectRow(this, tr("Outline Width"));
  outline_width = new DoubleField(outline_width_row, "outlinewidth");
  outline_width->SetColumnSpan(2);
  outline_width->SetMinimum(0);

  EffectRow* shadow_row = new EffectRow(this, tr("Shadow"));
  shadow_bool = new BoolField(shadow_row, "shadow");
  shadow_bool->SetColumnSpan(2);

  EffectRow* shadow_color_row = new EffectRow(this, tr("Shadow Color"));
  shadow_color = new ColorField(shadow_color_row, "shadowcolor");
  shadow_color->SetColumnSpan(2);

  EffectRow* shadow_angle_row = new EffectRow(this, tr("Shadow Angle"));
  shadow_angle = new DoubleField(shadow_angle_row, "shadowangle");
  shadow_angle->SetColumnSpan(2);

  EffectRow* shadow_distance_row = new EffectRow(this, tr("Shadow Distance"));
  shadow_distance = new DoubleField(shadow_distance_row, "shadowdistance");
  shadow_distance->SetColumnSpan(2);
  shadow_distance->SetMinimum(0);

  EffectRow* shadow_softness_row = new EffectRow(this, tr("Shadow Softness"));
  shadow_softness = new DoubleField(shadow_softness_row, "shadowsoftness");
  shadow_softness->SetColumnSpan(2);
  shadow_softness->SetMinimum(0);

  EffectRow* shadow_opacity_row = new EffectRow(this, tr("Shadow Opacity"));
  shadow_opacity = new DoubleField(shadow_opacity_row, "shadowopacity");
  shadow_opacity->SetColumnSpan(2);
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

void TextEffect::redraw(double timecode) {
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

  path.translate(position_x->GetDoubleAt(timecode) + padding, position_y->GetDoubleAt(timecode) + padding);

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
  close();

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
