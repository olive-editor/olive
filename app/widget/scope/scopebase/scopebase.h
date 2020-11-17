/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef SCOPEBASE_H
#define SCOPEBASE_H

#include "codec/frame.h"
#include "render/colorprocessor.h"
#include "widget/manageddisplay/manageddisplay.h"

OLIVE_NAMESPACE_ENTER

class ScopeBase : public ManagedDisplayWidget
{
public:
  ScopeBase(QWidget* parent = nullptr);

  virtual ~ScopeBase() override;

public slots:
  void SetBuffer(Frame* frame);

protected slots:
  virtual void OnInit() override;

  virtual void OnPaint() override;

  virtual void OnDestroy() override;

protected:
  virtual void showEvent(QShowEvent* e) override;

  virtual ShaderCode GenerateShaderCode() = 0;

  /**
   * @brief Draw function
   *
   * Override this if your sub-class scope needs extra drawing.
   */
  virtual void DrawScope(TexturePtr managed_tex, QVariant pipeline);

private:
  void UploadTextureFromBuffer();

  QVariant pipeline_;

  TexturePtr texture_;

  TexturePtr managed_tex_;

  Frame* buffer_;

};

OLIVE_NAMESPACE_EXIT

#endif // SCOPEBASE_H
