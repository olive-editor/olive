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

#ifndef GENERATEJOB_H
#define GENERATEJOB_H

#include "acceleratedjob.h"
#include "render/videoparams.h"

namespace olive {

class GenerateJob : public AcceleratedJob {
public:
  enum AlphaChannelSetting {
    kAlphaAuto,
    kAlphaForceOn,
    kAlphaForceOff
  };

  GenerateJob()
  {
    alpha_channel_required_ = kAlphaAuto;
    requested_format_ = VideoParams::kFormatInvalid;
  }

  AlphaChannelSetting GetAlphaChannelRequired() const { return alpha_channel_required_; }

  void SetAlphaChannelRequired(AlphaChannelSetting e) { alpha_channel_required_ = e; }

  VideoParams::Format GetRequestedFormat() const { return requested_format_; }

  void SetRequestedFormat(VideoParams::Format f) { requested_format_ = f; }

  const QString &GetColorspace() const { return colorspace_; }
  void SetColorspace(const QString &s) { colorspace_ = s; }

private:
  AlphaChannelSetting alpha_channel_required_;

  VideoParams::Format requested_format_;

  QString colorspace_;

};

}

Q_DECLARE_METATYPE(olive::GenerateJob)

#endif // GENERATEJOB_H
