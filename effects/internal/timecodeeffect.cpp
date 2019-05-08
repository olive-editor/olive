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
#include "global/timing.h"
#include "ui/comboboxex.h"
#include "ui/colorbutton.h"
#include "global/config.h"

TimecodeEffect::TimecodeEffect(Clip* c) :
  OldEffectNode(c)
{
  SetFlags(OldEffectNode::SuperimposeFlag);

  tc_select = new ComboInput(this, "tc_selector", tr("Timecode"));
  tc_select->AddItem(tr("Sequence"), true);
  tc_select->AddItem(tr("Media"), false);
  tc_select->SetValueAt(0, true);

  scale_val = new DoubleInput(this, "scale", tr("Scale"));
  scale_val->SetMinimum(1);
  scale_val->SetDefault(100);
  scale_val->SetMaximum(1000);

  color_val = new ColorInput(this, "color", tr("Color"));
  color_val->SetValueAt(0, QColor(Qt::white));

  color_bg_val = new ColorInput(this, "bgcolor", tr("Background Color"));
  color_bg_val->SetValueAt(0, QColor(Qt::black));

  bg_alpha = new DoubleInput(this, "bgalpha", tr("Background Opacity"));
  bg_alpha->SetMinimum(0);
  bg_alpha->SetDefault(50);
  bg_alpha->SetMaximum(100);

  offset_val = new Vec2Input(this, "offset", tr("Offset"));
  offset_val->SetDefault(0);

  prepend_text = new StringInput(this, "prepend", tr("Prepend"), false);
}

QString TimecodeEffect::name()
{
  return tr("Timecode");
}

QString TimecodeEffect::id()
{
  return "org.olivevideoeditor.Olive.timecode";
}

QString TimecodeEffect::category()
{
  return tr("Render");
}

QString TimecodeEffect::description()
{
  return tr("Render the media or sequence timecode on this clip.");
}

EffectType TimecodeEffect::type()
{
  return EFFECT_TYPE_EFFECT;
}

olive::TrackType TimecodeEffect::subtype()
{
  return olive::kTypeVideo;
}

OldEffectNodePtr TimecodeEffect::Create(Clip *c)
{
  return std::make_shared<TimecodeEffect>(c);
}


void TimecodeEffect::redraw(double timecode) {
  Sequence* sequence = parent_clip->track()->sequence();

  if (tc_select->GetValueAt(timecode).toBool()) {
    display_timecode = prepend_text->GetStringAt(timecode) + frame_to_timecode(sequence->playhead,
                                                                               olive::config.timecode_view,
                                                                               sequence->frame_rate());
  } else {
    double media_rate = parent_clip->media_frame_rate();
    display_timecode = prepend_text->GetStringAt(timecode) + frame_to_timecode(qRound(timecode * media_rate),
                                                                               olive::config.timecode_view,
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

  int text_x, text_y, rect_y;
  int text_height = fm.height();
  int text_width = fm.width(display_timecode);
  QColor background_color = color_bg_val->GetColorAt(timecode);
  int alpha_val = qCeil(bg_alpha->GetDoubleAt(timecode)*2.55);
  background_color.setAlpha(alpha_val);

  QVector2D offset = offset_val->GetVector2DAt(timecode);

  text_x = offset.x() + (width/2) - (text_width/2);
  text_y = offset.y() + height - height/10;
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
