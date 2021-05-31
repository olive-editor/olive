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

#ifndef HISTOGRAMSCOPE_H
#define HISTOGRAMSCOPE_H

#include "widget/scope/scopebase/scopebase.h"

namespace olive {

class HistogramScope : public ScopeBase
{
  Q_OBJECT
public:
  HistogramScope(QWidget* parent = nullptr);

  MANAGEDDISPLAYWIDGET_DEFAULT_DESTRUCTOR(HistogramScope)

protected slots:
  virtual void OnInit() override;

  virtual void OnDestroy() override;

protected:
  virtual ShaderCode GenerateShaderCode() override;
  QVariant CreateSecondaryShader();

  virtual void DrawScope(TexturePtr managed_tex, QVariant pipeline) override;

private:
  QVariant pipeline_secondary_;
  TexturePtr texture_row_sums_;

};

}

#endif // HISTOGRAMSCOPE_H
