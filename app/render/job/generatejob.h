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
  }

  AlphaChannelSetting GetAlphaChannelRequired() const
  {
    return alpha_channel_required_;
  }

  void SetAlphaChannelRequired(AlphaChannelSetting e)
  {
    alpha_channel_required_ = e;
  }

private:
  AlphaChannelSetting alpha_channel_required_;

};

}

Q_DECLARE_METATYPE(olive::GenerateJob)

#endif // GENERATEJOB_H
