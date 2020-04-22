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

#include "exportparams.h"

OLIVE_NAMESPACE_ENTER

ExportParams::ExportParams() :
  video_scaling_method_(kStretch),
  has_custom_range_(false)
{
}

const QString &ExportParams::encoder() const
{
  return encoder_id_;
}

void ExportParams::set_encoder(const QString &id)
{
  encoder_id_ = id;
}

bool ExportParams::has_custom_range() const
{
  return has_custom_range_;
}

const TimeRange &ExportParams::custom_range() const
{
  return custom_range_;
}

void ExportParams::set_custom_range(const TimeRange &custom_range)
{
  has_custom_range_ = true;
  custom_range_ = custom_range;
}

const ExportParams::VideoScalingMethod &ExportParams::video_scaling_method() const
{
  return video_scaling_method_;
}

void ExportParams::set_video_scaling_method(const ExportParams::VideoScalingMethod &video_scaling_method)
{
  video_scaling_method_ = video_scaling_method;
}

const ColorTransform &ExportParams::color_transform() const
{
  return color_transform_;
}

void ExportParams::set_color_transform(const ColorTransform &color_transform)
{
  color_transform_ = color_transform;
}

OLIVE_NAMESPACE_EXIT
