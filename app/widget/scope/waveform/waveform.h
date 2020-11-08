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

#ifndef WAVEFORMSCOPE_H
#define WAVEFORMSCOPE_H

#include "widget/scope/scopebase/scopebase.h"
#include <QCheckBox>
#include <QVector>

OLIVE_NAMESPACE_ENTER

class WaveformScope : public ScopeBase
{
  Q_OBJECT
public:
  WaveformScope(QWidget* parent = nullptr);

protected:
  virtual OpenGLShaderPtr CreateShader() override;

  virtual void DrawScope() override;

private:
  virtual void SetupControlUI() override;

  void SortSwizzleData(int checkbox);

  QCheckBox* luma_select_;

  QCheckBox* red_select_;

  QCheckBox* green_select_;

  QCheckBox* blue_select_;

  QVector<bool> swizzle_;

};

OLIVE_NAMESPACE_EXIT

#endif // WAVEFORMSCOPE_H
