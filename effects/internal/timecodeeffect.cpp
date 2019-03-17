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

#include "timecodeeffect.h"

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
#include <QDebug>

#include "ui/labelslider.h"
#include "ui/collapsiblewidget.h"
#include "timeline/clip.h"
#include "timeline/sequence.h"
#include "panels/viewer.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "ui/fontcombobox.h"
#include "global/config.h"

TimecodeEffect::TimecodeEffect(Clip* c, const EffectMeta* em) :
  Effect(c, em)
{
  SetFlags(Effect::SuperimposeFlag);

  EffectRow* tc_row = new EffectRow(this, tr("Timecode"));
  tc_select = new ComboField(tc_row, "tc_selector");
  tc_select->AddItem(tr("Sequence"), olive::effect::sequence);
  tc_select->AddItem(tr("Media"), olive::effect::media);
  //check if clip has media, if it does then find out if that media is a sequence
  //as long as the type is not sequence add source timecode selector
  if(parent_clip->media()){
    if(!(parent_clip->media()->get_type() == MEDIA_TYPE_SEQUENCE)){
                tc_select->AddItem(tr("Source"), olive::effect::source);
    }
  }
  tc_select->SetValueAt(0, olive::effect::sequence);

  EffectRow* scale_row = new EffectRow(this, tr("Scale"));
  scale_val = new DoubleField(scale_row, "scale");
  scale_val->SetColumnSpan(2);
  scale_val->SetMinimum(1);
  scale_val->SetDefault(100);
  scale_val->SetMaximum(1000);

  EffectRow* color_row = new EffectRow(this, tr("Color"));
  color_val = new ColorField(color_row, "color");
  color_val->SetColumnSpan(2);
  color_val->SetValueAt(0, QColor(Qt::white));

  EffectRow* color_bg_row = new EffectRow(this, tr("Background Color"));
  color_bg_val = new ColorField(color_bg_row, "bgcolor");
  color_bg_val->SetColumnSpan(2);
  color_bg_val->SetValueAt(0, QColor(Qt::black));

  EffectRow* bg_alpha_row = new EffectRow(this, tr("Background Opacity"));
  bg_alpha = new DoubleField(bg_alpha_row, "bgalpha");
  bg_alpha->SetColumnSpan(2);
  bg_alpha->SetMinimum(0);
  bg_alpha->SetDefault(50);
  bg_alpha->SetMaximum(100);

  EffectRow* offset_row = new EffectRow(this, tr("Offset"));
  offset_x_val = new DoubleField(offset_row, "offsetx");
  offset_y_val = new DoubleField(offset_row, "offsety");

  EffectRow* prepent_text_row = new EffectRow(this, tr("Prepend"));
  prepend_text = new StringField(prepent_text_row, "prepend");
  prepend_text->SetColumnSpan(2);
}

void TimecodeEffect::redraw(double timecode) {
  double media_rate = 0;
  double timecode_start = 0;

  switch (tc_select->GetValueAt(timecode).toInt()){
    case olive::effect::sequence:
      display_timecode = prepend_text->GetStringAt(timecode) + frame_to_timecode(olive::ActiveSequence->playhead,
                                                                                 olive::CurrentConfig.timecode_view,
                                                                                 olive::ActiveSequence->frame_rate);
      break;
    case olive::effect::source:
      media_rate = parent_clip->media_frame_rate();
      timecode_start = timecode_to_frame(parent_clip->media()->to_footage()->video_tracks.at(0).timecode_source_start,
                                         olive::CurrentConfig.timecode_view,
                                         media_rate);
      display_timecode = prepend_text->GetStringAt(timecode) + frame_to_timecode((timecode * media_rate) + timecode_start,
                                                                                 olive::CurrentConfig.timecode_view,
                                                                                 media_rate);
      break;
    case olive::effect::media:
      media_rate = parent_clip->media_frame_rate();
      display_timecode = prepend_text->GetStringAt(timecode) + frame_to_timecode(timecode * media_rate,
                                                                                 olive::CurrentConfig.timecode_view,
                                                                                 media_rate);
  }
  img.fill(Qt::transparent);

  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  int width = img.width();
  int height = img.height();

  // set font
  font.setStyleHint(QFont::Helvetica, QFont::PreferAntialias);
  font.setFamily("Helvetica");
  font.setPixelSize(qCeil(scale_val->GetDoubleAt(timecode)*.01*(height/10)));
  p.setFont(font);
  QFontMetrics fm(font);

  QPainterPath path;

  int text_x, text_y, rect_y, offset_x, offset_y;
  int text_height = fm.height();
  int text_width = fm.width(display_timecode);
  QColor background_color = color_bg_val->GetColorAt(timecode);
  int alpha_val = qCeil(bg_alpha->GetDoubleAt(timecode)*2.55);
  background_color.setAlpha(alpha_val);

  offset_x = int(offset_x_val->GetDoubleAt(timecode));
  offset_y = int(offset_y_val->GetDoubleAt(timecode));

  text_x = offset_x + (width/2) - (text_width/2);
  text_y = offset_y + height - height/10;
  rect_y = text_y + fm.descent() - text_height;

  path.addText(text_x, text_y, font, display_timecode);

  p.setPen(Qt::NoPen);
  p.setBrush(background_color);
  p.drawRect(QRect(text_x-fm.descent(), rect_y, text_width+fm.descent()*2, text_height));
  p.setBrush(color_val->GetColorAt(timecode));
  p.drawPath(path);
}

bool TimecodeEffect::AlwaysUpdate()
{
  return true;
}
