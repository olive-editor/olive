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

protected:
  virtual void initializeGL() override;

  virtual void paintGL() override;

  virtual void showEvent(QShowEvent* e) override;

  virtual QVariant CreateShader();

  virtual void DrawScope();

  QVariant pipeline()
  {
    return pipeline_;
  }

  QVariant managed_tex()
  {
    return managed_tex_;
  }

private:
  void UploadTextureFromBuffer();

  QVariant pipeline_;

  QVariant texture_;

  QVariant managed_tex_;

  Frame* buffer_;

private slots:
  void CleanUp();

};

OLIVE_NAMESPACE_EXIT

#endif // SCOPEBASE_H
