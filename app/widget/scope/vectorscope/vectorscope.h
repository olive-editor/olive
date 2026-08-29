/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2026 mikesolar

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

#ifndef VECTORSCOPESCOPE_H
#define VECTORSCOPESCOPE_H

#include "widget/scope/scopebase/scopebase.h"

namespace olive
{

class VectorscopeScope : public ScopeBase {
	Q_OBJECT
public:
	VectorscopeScope(QWidget *parent = nullptr);

	MANAGEDDISPLAYWIDGET_DEFAULT_DESTRUCTOR(VectorscopeScope)

protected:
	virtual ShaderCode GenerateShaderCode() override;

	virtual void DrawScope(TexturePtr managed_tex, QVariant pipeline) override;

	virtual void DrawScopeSoftware(QPainter &p, const QImage &image) override;
};

}

#endif // VECTORSCOPESCOPE_H
