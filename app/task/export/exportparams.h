/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef EXPORTPARAMS_H
#define EXPORTPARAMS_H

#include <QMatrix4x4>

#include "codec/encoder.h"
#include "node/output/viewer/viewer.h"
#include "render/colortransform.h"

namespace olive {

class ExportParams : public EncodingParams {
public:
  enum VideoScalingMethod {
    kFit,
    kStretch,
    kCrop
  };

  ExportParams();

  const Encoder::Type& encoder() const;
  void set_encoder(const Encoder::Type& id);

  bool has_custom_range() const;
  const TimeRange& custom_range() const;
  void set_custom_range(const TimeRange& custom_range);

  const VideoScalingMethod& video_scaling_method() const;
  void set_video_scaling_method(const VideoScalingMethod& video_scaling_method);

  const ColorTransform& color_transform() const;
  void set_color_transform(const ColorTransform& color_transform);

  static QMatrix4x4 GenerateMatrix(ExportParams::VideoScalingMethod method,
                                   int source_width, int source_height,
                                   int dest_width, int dest_height);

  virtual void Save(QXmlStreamWriter* writer) const override;

private:
  Encoder::Type encoder_id_;

  VideoScalingMethod video_scaling_method_;

  bool has_custom_range_;
  TimeRange custom_range_;

  ColorTransform color_transform_;

};

}

#endif // EXPORTPARAMS_H
