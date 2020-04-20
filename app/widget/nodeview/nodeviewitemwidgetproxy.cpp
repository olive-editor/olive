/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "nodeviewitemwidgetproxy.h"
#include <QDebug>

// NodeDefaultStyleWidget::NodeDefaultStyleWidget(QWidget* parent) :
//   QWidget(parent)
// {
// }

QColor NodeDefaultStyleWidget::TitleBarColor()
{
  return title_bar_color_;
}

void NodeDefaultStyleWidget::SetTitleBarColor(QColor color)
{
  title_bar_color_ = color;
}

QColor NodeDefaultStyleWidget::BorderColor()
{
  return border_color_;
}

void NodeDefaultStyleWidget::SetBorderColor(QColor color)
{
  border_color_ = color;
}

Q_GLOBAL_STATIC(NodeStylesRegistry, global_style_registry_)
NodeStylesRegistry* NodeStylesRegistry::GetRegistry(void)
{
  return (NodeStylesRegistry*)global_style_registry_;
}

NodeStylesRegistry::NodeStylesRegistry(QWidget* parent)
{
  registry_.insert(
    0,
    &default_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.audioinput",
    &input_media_audio_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.volume",
    &audio_volume_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.track",
    &output_track_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.vieweroutput",
    &output_viewer_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.clip",
    &block_clip_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.videoinput",
    &input_media_video_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.transform",
    &distort_transform_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.pan",
    &audio_pan_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.gap",
    &block_gap_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.math",
    &math_math_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.trigonometry",
    &math_trigonometry_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.solidgenerator",
    &generator_solid_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.alphaoverblend",
    &manipulation_merge_style_
  );
  registry_.insert(
    "org.olivevideoeditor.Olive.blur",
    &convolution_blur_style_
  );
}

NodeDefaultStyleWidget* NodeStylesRegistry::GetStyle(QString id)
{
  return registry_.value(id, &default_style_);
}