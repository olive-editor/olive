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

#ifndef WAVEFORMSCOPE_H
#define WAVEFORMSCOPE_H

#include "widget/scope/scopebase/scopebase.h"

namespace olive {

class WaveformScope : public ScopeBase
{
  Q_OBJECT
public:
  WaveformScope(QWidget* parent = nullptr);

  MANAGEDDISPLAYWIDGET_DEFAULT_DESTRUCTOR(WaveformScope)

protected:
  virtual ShaderCode GenerateShaderCode() override;

  virtual void DrawScope(TexturePtr managed_tex, QVariant pipeline) override;

};

}

#endif // WAVEFORMSCOPE_H
