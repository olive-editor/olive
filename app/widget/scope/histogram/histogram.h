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

#ifndef HISTOGRAMSCOPE_H
#define HISTOGRAMSCOPE_H

#include "widget/scope/scopebase/scopebase.h"

OLIVE_NAMESPACE_ENTER

class HistogramScope : public ScopeBase
{
  Q_OBJECT
public:
  HistogramScope(QWidget* parent = nullptr);

  virtual ~HistogramScope() override;

public slots:
  void PerformanceMode();

protected:
  virtual void initializeGL() override;

  virtual OpenGLShaderPtr CreateShader() override;
  OpenGLShaderPtr CreateSecondaryShader();

  void AssertAdditionalTextures();

  virtual void DrawScope() override;

private:
  OpenGLShaderPtr pipeline_secondary_;
  OpenGLTexture texture_row_sums_;

  bool performance_mode_;

private slots:
  void CleanUp();
};

OLIVE_NAMESPACE_EXIT

#endif // HISTOGRAMSCOPE_H
