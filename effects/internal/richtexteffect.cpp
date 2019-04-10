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

#include "richtexteffect.h"

#include <QTextDocument>
#include <QtMath>

#include "timeline/clip.h"
#include "ui/blur.h"

enum AutoscrollDirection {
  SCROLL_OFF,
  SCROLL_UP,
  SCROLL_DOWN,
  SCROLL_LEFT,
  SCROLL_RIGHT,
};

RichTextEffect::RichTextEffect(Clip *c, const EffectMeta *em) :
  Effect(c, em)
{
  SetFlags(Effect::SuperimposeFlag);

  text_val = new StringInput(this, "text", tr("Text"));

  padding_field = new DoubleInput(this, "padding", tr("Padding"));

  position = new Vec2Input(this, "pos", tr("Position"));

  vertical_align = new ComboInput(this, "valign", tr("Vertical Align:"));
  vertical_align->AddItem(tr("Top"), Qt::AlignTop);
  vertical_align->AddItem(tr("Center"), Qt::AlignCenter);
  vertical_align->AddItem(tr("Bottom"), Qt::AlignBottom);
  vertical_align->SetValueAt(0, Qt::AlignCenter);

  autoscroll = new ComboInput(this, "autoscroll", tr("Auto-Scroll"));
  autoscroll->AddItem(tr("Off"), SCROLL_OFF);
  autoscroll->AddItem(tr("Up"), SCROLL_UP);
  autoscroll->AddItem(tr("Down"), SCROLL_DOWN);
  autoscroll->AddItem(tr("Left"), SCROLL_LEFT);
  autoscroll->AddItem(tr("Right"), SCROLL_RIGHT);

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

  // Create default text
  text_val->SetValueAt(0, "<html>"
                            "<body style=\"color: #ffffff; font-size: 36pt;\">"
                              "<center>Sample Text</center>"
                            "</body>"
                          "</html>");
}

void RichTextEffect::redraw(double timecode)
{
  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing);
  int width = img.width();
  int height = img.height();

  int padding = qRound(padding_field->GetDoubleAt(timecode));

  width -= 2 * padding;
  height -= 2 * padding;

  QTextDocument td;
  td.setHtml(text_val->GetStringAt(timecode));
  td.setTextWidth(width);

  QPoint translation = position->GetVector2DAt(timecode).toPoint();
  translation += {padding, padding};

  int doc_height = qRound(td.size().height());

  AutoscrollDirection auto_scroll_dir = static_cast<AutoscrollDirection>(autoscroll->GetValueAt(timecode).toInt());

  double scroll_progress = 0;

  if (auto_scroll_dir != SCROLL_OFF) {
    double clip_length_secs = double(parent_clip->length()) / parent_clip->media_frame_rate();
    scroll_progress = (timecode - double(parent_clip->clip_in()) / parent_clip->media_frame_rate()) / clip_length_secs;
  }

  if (auto_scroll_dir == SCROLL_OFF || auto_scroll_dir == SCROLL_LEFT || auto_scroll_dir == SCROLL_RIGHT) {

    // If we're not auto-scrolling the vertical direction, respect the vertical alignment
    if (vertical_align->GetValueAt(timecode).toInt() == Qt::AlignCenter) {
      translation.setY(translation.y() + height / 2 - doc_height / 2);
    } else if (vertical_align->GetValueAt(timecode).toInt() == Qt::AlignBottom) {
      translation.setY(translation.y() + height - doc_height);
    }

    // Check if we are autoscrolling
    if (auto_scroll_dir != SCROLL_OFF) {

      if (auto_scroll_dir == SCROLL_LEFT) {
        scroll_progress = 1.0 - scroll_progress;
      }

      int doc_width = qRound(td.size().width());
      translation.setX(translation.x() + qRound(-doc_width + (img.width() + doc_width) * scroll_progress));
    }

  } else if (auto_scroll_dir == SCROLL_UP || auto_scroll_dir == SCROLL_DOWN) {

    // Auto-scroll bottom to top or top to bottom

    if (auto_scroll_dir == SCROLL_UP) {
      scroll_progress = 1.0 - scroll_progress;
    }

    translation.setY(translation.y() + qRound(-doc_height + (img.height() + doc_height)*scroll_progress));

  }

  QRect clip_rect = img.rect();
  clip_rect.translate(-translation);
  p.translate(translation);

  img.fill(Qt::transparent);

  // draw software shadow
  if (shadow_bool->GetBoolAt(timecode)) {

    // calculate offset using distance and angle
    double angle = shadow_angle->GetDoubleAt(timecode) * M_PI / 180.0;
    double distance = qFloor(shadow_distance->GetDoubleAt(timecode));
    int shadow_x_offset = qRound(qCos(angle) * distance);
    int shadow_y_offset = qRound(qSin(angle) * distance);

    p.translate(shadow_x_offset, shadow_y_offset);
    clip_rect.translate(-shadow_x_offset, -shadow_y_offset);

    td.drawContents(&p, clip_rect);

    int blurSoftness = qFloor(shadow_softness->GetDoubleAt(timecode));
    if (blurSoftness > 0) {
      olive::ui::blur(img, img.rect(), blurSoftness, true);
    }

    p.setCompositionMode(QPainter::CompositionMode_SourceIn);

    p.fillRect(img.rect(), shadow_color->GetColorAt(timecode));

    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    p.translate(-shadow_x_offset, -shadow_y_offset);
    clip_rect.translate(shadow_x_offset, shadow_y_offset);
  }

  td.drawContents(&p, clip_rect);

  p.end();
}

bool RichTextEffect::AlwaysUpdate()
{
  return autoscroll->GetValueAt(Now()).toInt() != SCROLL_OFF;
}
